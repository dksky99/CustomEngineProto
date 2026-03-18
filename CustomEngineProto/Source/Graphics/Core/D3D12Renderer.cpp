#include "D3D12Renderer.h" // 헤더를 포함합니다.
#include <d3dcompiler.h> // 셰이더 컴파일을 위한 헤더를 포함합니다.
#include <vector> // C++의 동적 배열인 std::vector를 사용하기 위해 포함합니다.
#include <algorithm> 



#include "Graphics/Resources/Mesh.h" // 분리된 메시 구조체를 사용하기 위해 포함합니다.
// 렌더러가 씬과 액터의 정보를 파악할 수 있도록 방금 만든 프레임워크 헤더 파일들을 참조합니다. 

#include "Framework/Core/Scene.h" // 렌더러가 화면에 그릴 타겟 씬(맵)을 파악하기 위함입니다.
#include "Framework/Core/Actor.h" // 씬 안에 들어있는 액터들의 위치(Transform)를 빼오기 위함입니다.
#include "Framework/Components/MeshComponent.h" //  액터가 메시 부품을 달고 있는지 검사하기 위해 포함합니다. 
#include "Framework/Core/Camera.h" 

// 소스 코드 내에서 링커에게 컴파일러 라이브러리를 연결하라고 지시합니다.
#pragma comment(lib, "d3dcompiler.lib") 
#pragma comment(lib, "dxguid.lib") // DX12의 인터페이스 ID(GUID)들이 정의된 라이브러리를 연결합니다.

D3D12Renderer::D3D12Renderer() // 생성자 구현부입니다.
    : mClientWidth(0), mClientHeight(0), mMainWindow(nullptr) // 멤버 변수들을 0과 nullptr로 안전하게 초기화합니다.
{ // 생성자 본문 시작
} // 생성자 본문 끝

D3D12Renderer::~D3D12Renderer() // 소멸자 구현부입니다.
{ // 소멸자 본문 시작
    // ComPtr을 사용하므로 수동으로 메모리 해제(Release)를 할 필요가 없어 비워둡니다.
} // 소멸자 본문 끝

bool D3D12Renderer::Initialize(HWND hwnd, int width, int height) // DX12 초기화 함수의 구현부입니다.
{ // Initialize 함수 시작
    mMainWindow = hwnd; // 매개변수로 받은 윈도우 핸들을 멤버 변수에 저장합니다.
    mClientWidth = width; // 창의 너비를 저장합니다.
    mClientHeight = height; // 창의 높이를 저장합니다.







    // ------------------------------------------------

    // 뷰포트 설정: 렌더링될 화면의 범위를 전체 창 크기로 지정합니다.
    mViewport.TopLeftX = 0.0f; // 좌상단 X 좌표
    mViewport.TopLeftY = 0.0f; // 좌상단 Y 좌표
    mViewport.Width = static_cast<float>(width); // 가로 길이
    mViewport.Height = static_cast<float>(height); // 세로 길이
    mViewport.MinDepth = 0.0f; // 3D 깊이 최솟값
    mViewport.MaxDepth = 1.0f; // 3D 깊이 최댓값

    // 가위(Scissor) 사각형 설정: 이 영역 바깥은 렌더링하지 않습니다.
    mScissorRect = { 0, 0, width, height }; // 전체 창 크기 지정

#if defined(_DEBUG) // 현재 빌드 모드가 디버그(Debug) 모드일 때만 실행되는 전처리 구문입니다.
    ComPtr<ID3D12Debug> debugController; // DX12의 오류를 잡아줄 디버그 컨트롤러 객체를 선언합니다.
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) // 디버그 인터페이스를 성공적으로 가져왔다면,
    { // 디버그 활성화 블록 시작
        debugController->EnableDebugLayer(); // DX12 디버그 레이어를 켜서 잘못된 코드를 짰을 때 출력창에 경고를 띄우게 합니다.
    } // 디버그 활성화 블록 끝
#endif // 디버그 모드 전처리 구문 끝

    // 1. DXGI 팩토리 생성
    CreateDXGIFactory1(IID_PPV_ARGS(&mDxgiFactory));

    // 2. D3D12 디바이스 생성
    HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice));
    if (FAILED(hr)) return false;

    // 3. 커맨드 큐 생성
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));

    // 4. 스왑 체인 생성
    DXGI_SWAP_CHAIN_DESC1 sd = {};
    sd.Width = mClientWidth;
    sd.Height = mClientHeight;
    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.Stereo = FALSE;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SwapChainBufferCount;
    sd.Scaling = DXGI_SCALING_STRETCH;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ComPtr<IDXGISwapChain1> swapChain1;
    mDxgiFactory->CreateSwapChainForHwnd(
        mCommandQueue.Get(), mMainWindow, &sd, nullptr, nullptr, &swapChain1
    );
    swapChain1.As(&mSwapChain);

    // 5. 커맨드 할당자 생성
    mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator));

    // 6. 커맨드 리스트 생성
    mDevice->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList)
    );
    mCommandList->Close();

    // 7. 동기화 객체(Fence) 생성
    mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
    mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mCbvSrvUavDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); //  서랍장 크기 캐싱

    // 8. RTV 디스크립터 힙 생성
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap));

    // 9. 렌더 타겟 뷰(RTV) 생성
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());

    for (UINT i = 0; i < SwapChainBufferCount; i++)
    {
        mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i]));
        mDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, mRtvDescriptorSize);
    }


    // --- 10. 깊이-스텐실 서랍장(DSV Heap) 생성 ---
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1; // 깊이 버퍼는 1개만 있으면 됩니다.
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // 깊이-스텐실 뷰 타입입니다.
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // 셰이더에서 직접 보지 않으므로 NONE 입니다.
    dsvHeapDesc.NodeMask = 0;
    mDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap)); // 서랍장 객체를 생성합니다.


    // 11. 깊이 버퍼 리소스(2D 텍스처) 생성 및 DSV 연결
    D3D12_RESOURCE_DESC depthStencilDesc = {}; // 깊이 버퍼 텍스처의 속성을 정의합니다.
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2D 텍스처 형태입니다.
    depthStencilDesc.Alignment = 0; // 기본 정렬
    depthStencilDesc.Width = mClientWidth; // 텍스처 너비는 화면 너비와 동일합니다.
    depthStencilDesc.Height = mClientHeight; // 텍스처 높이는 화면 높이와 동일합니다.
    depthStencilDesc.DepthOrArraySize = 1; // 1장의 텍스처
    depthStencilDesc.MipLevels = 1; // 밉맵 사용 안 함
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 24비트는 깊이(거리), 8비트는 스텐실(마스크)로 사용합니다.
    depthStencilDesc.SampleDesc.Count = 1; // MSAA 미사용
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // 최적 레이아웃은 드라이버에 맡깁니다.
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // 이 텍스처를 깊이-스텐실 용도로 허용합니다.


    // 화면 지우기(Clear) 성능 최적화를 위해 깊이 버퍼의 초기값을 명시합니다. (깊이 1.0 = 가장 먼 곳)
    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 위와 포맷을 맞춥니다.
    optClear.DepthStencil.Depth = 1.0f; // 1.0f는 가장 멀리 있는 상태를 뜻합니다.
    optClear.DepthStencil.Stencil = 0;

    CD3DX12_HEAP_PROPERTIES depthHeapProps(D3D12_HEAP_TYPE_DEFAULT); // 가장 빠른 기본(Default) 메모리 풀을 사용합니다.
    mDevice->CreateCommittedResource(
        &depthHeapProps, D3D12_HEAP_FLAG_NONE, &depthStencilDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, // 이 버퍼에 깊이 값을 기록(Write)할 수 있는 상태로 생성합니다.
        &optClear, IID_PPV_ARGS(&mDepthStencilBuffer)); // GPU 메모리에 최종 할당합니다.

    // 할당된 GPU 텍스처 메모리를 아까 만든 서랍장(DSV Heap) 첫 번째 칸에 예쁘게 꽂아 넣습니다.
    mDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, mDsvHeap->GetCPUDescriptorHandleForHeapStart());
    // -------------------------------------------------------------

    // --- 엔진 파이프라인 및 데이터 초기화 핵심 ---
    if (!BuildRootSignature()) return false; // 1. 루트 시그니처 세팅
    if (!BuildPSO()) return false;           // 2. 파이프라인 상태 객체 세팅
    // 기하학 데이터를 세팅하기 전에, 텍스처 이미지를 먼저 생성해서 메모리에 올려줍니다!
    if (!BuildTexture()) return false;
    //if (!BuildGeometry()) return false;      // 3. 기하학(버텍스+인덱스) 데이터 세팅
     //  렌더러가 길게 쓰던 기하학 생성 함수(BuildGeometry) 대신, 외부 메시 클래스를 생성하고 위임합니다! 
    mDefaultBoxMesh = std::make_shared<Mesh>(); // 렌더러 소유의 기본 박스 메시를 동적 생성합니다.
    mDefaultBoxMesh->CreateBox(mDevice.Get()); // 메시 객체 스스로 GPU 메모리를 할당하여 모양을 만들게 지시합니다.
    //  ========================================================================= 
    //   [구조 분화] 변경된 초기화 함수들을 호출합니다.  
    if (!BuildOffscreenRenderTargets()) return false;
    if (!BuildConstantBuffers()) return false;
    if (!BuildPostProcessPipelines()) return false;


    return true; // 초기화 완벽히 성공!
} // Initialize 함수의 끝

// --- Update 함수 구현 ---
void D3D12Renderer::Update(float deltaTime, Scene* scene,Camera* camera)
{
    

   

    // ====================================================================
    // [3단계: 파이프라인 전송 (W * V * P)]
    // 과거의 1개짜리 큐브 로직을 모두 지우고, 100개짜리로 완전히 통일했습니다!
    // ====================================================================
    static float totalTime = 0.0f;
    totalTime += deltaTime;
    // 렌더러가 직접 계산하던 뷰와 투영 행렬을, 카메라 객체에게서 우아하게 꺼내옵니다. 
    XMMATRIX view = camera->GetView(); // 카메라가 세팅해둔 시점(View) 행렬을 받습니다.
    XMMATRIX proj = camera->GetProj(); // 카메라가 세팅해둔 원근감(Proj) 행렬을 받습니다.
    XMMATRIX viewProj = view * proj; // V*P 연산을 수행하여 최종 압축 행렬을 만듭니다.

    PassConstants passConstants;
    XMStoreFloat4x4(&passConstants.ViewProj, XMMatrixTranspose(viewProj));

    //  동적 조명(Dynamic Lighting) 시뮬레이션! 
    // 시간에 따라 태양이 원을 그리며 도는 궤적을 계산합니다.
    float sunSpeed = 0.5f; // 태양이 도는 속도입니다.
    float sunX = cosf(totalTime * sunSpeed); // 시간에 따라 좌우(X축)로 움직입니다.
    float sunY = sinf(totalTime * sunSpeed); // 시간에 따라 상하(Y축, 고도)로 움직입니다.
    float sunZ = 0.5f; // 약간 비스듬하게 비추도록 Z축 고정값을 줍니다.

    // 계산된 위치로 빛의 방향 벡터를 만듭니다. (빛이 쏟아지는 방향)
    XMVECTOR lightDir = XMVectorSet(sunX, sunY, sunZ, 0.0f);
    lightDir = XMVector3Normalize(lightDir); // 방향 벡터이므로 길이를 1로 정규화합니다.
    XMStoreFloat3(&passConstants.LightDir, lightDir); // 상수 버퍼 구조체에 넣습니다.

    // 태양의 고도(sunY)에 따라 빛의 색상과 밝기를 바꿉니다. (일몰/일출 효과)
    // 태양이 가장 높을 때(sunY=1) 가장 밝고, 지평선 아래(sunY<0)로 떨어지면 어두워집니다.
    float intensity = std::clamp(sunY + 0.2f, 0.0f, 1.0f); // 빛의 세기를 0.0 ~ 1.0 사이로 제한합니다.

    // 약간의 노을 느낌을 주기 위해 빨간색(R)은 유지하고 초록(G), 파랑(B) 값을 조금 깎아냅니다.
    passConstants.LightColor = { intensity, intensity * 0.9f, intensity * 0.8f, 1.0f };

    //  [ ] C++ 코드는 이 두 줄만 추가되면 완벽합니다! 
    passConstants.TotalTime = totalTime; // 현재 흐른 누적 시간을 셰이더에게 전달합니다. (UV를 움직일 때 사용)
    // 렌더러가 직접 들고 있던 위치 대신, 카메라 객체에게 "너 지금 3D 위치가 어디야?" 하고 물어서 위치값을 가져와 광택 연산용으로 포장합니다. 
    passConstants.EyePosW = camera->GetPosition();

    memcpy(mMappedPassCB, &passConstants, sizeof(PassConstants));
    //  렌더 배칭(Render Batching)의 기초! 모든 액터를 무식하게 다 그리지 않고 필터링합니다. 
    mInstanceCountToDraw = 0; // 이번 프레임에 그려야 할 인스턴스 카운트를 0으로 초기화합니다.
    const auto& actors = scene->GetActors(); // 씬에서 모든 액터 배열을 가져옵니다.

    for (auto& actor : actors) // 세상의 모든 액터를 순회합니다.
    { // 루프 시작
        // 해당 액터가 '외형 부품(MeshComponent)'을 장착하고 있는지 검색해서 포인터를 가져옵니다.
        auto meshComp = actor->GetComponent<MeshComponent>();

        // 만약 메시 컴포넌트가 있고, 그 컴포넌트에 빈 껍데기가 아닌 진짜 3D 모델(Mesh)이 꽂혀 있다면!
        if (meshComp && meshComp->GetMesh())
        { // 렌더링 합격 블록
            XMMATRIX finalWorld = actor->GetTransform()->GetWorldMatrix(); // 액터의 3D 공간 행렬을 뽑아냅니다.

            // GPU 인스턴스 명부의 비어있는 다음 칸에 이 행렬을 덮어씌웁니다.
            XMStoreFloat4x4(&mMappedInstanceData[mInstanceCountToDraw].World, XMMatrixTranspose(finalWorld));

            mInstanceCountToDraw++; // 그려야 할 목록 1명 추가요!

            if (mInstanceCountToDraw >= NumInstances) break; // 최대치(100)를 넘으면 엔진 뻗는 걸 막기 위해 탈출합니다.
        } // 합격 블록 끝
    } // 루프 끝
}

//   [구조 분화] 엔진의 메인 Draw 함수가 구조적으로 매우 깔끔해졌습니다!  
void D3D12Renderer::Draw()
{
    mCommandAllocator->Reset();
    mCommandList->Reset(mCommandAllocator.Get(), mPSO.Get());

    // 1. 3D 씬을 임시 도화지(Ping)에 렌더링합니다.
    RenderScene();

    // 2. 임시 도화지를 재료로 삼아 2D 필터(포스트 프로세스 체인)를 적용합니다.
    RenderPostProcess();

    // 3. 모든 명령을 닫고 제출합니다.
    mCommandList->Close();
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, cmdsLists);

    mSwapChain->Present(0, 0);
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    FlushCommandQueue();
}

void D3D12Renderer::FlushCommandQueue()
{
    mCurrentFence++;
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);

    if (mFence->GetCompletedValue() < mCurrentFence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        mFence->SetEventOnCompletion(mCurrentFence, eventHandle);
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

bool D3D12Renderer::BuildRootSignature()
{
    // 새로운 셰이더 규칙(레지스터)에 맞게 계약서(루트 시그니처)를 완전히 새로 작성합니다! 

    // 1번 조항: 공통(Pass) 상수 버퍼 (b0) - 서랍장 형태
    CD3DX12_DESCRIPTOR_RANGE cbvPassTable;
    cbvPassTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // b0


    // 3번 조항: 텍스처 (t0) - 서랍장 형태
    CD3DX12_DESCRIPTOR_RANGE srvTable;
    srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0
    // 이제 고유 데이터는 CBV(상수 버퍼)가 아니라 SRV(배열 버퍼)로 들어옵니다! 
    CD3DX12_DESCRIPTOR_RANGE srvInstanceTable;
    srvInstanceTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // t1 (Instance Array)

    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};
    rootParameters[0].InitAsDescriptorTable(1, &cbvPassTable);
    rootParameters[1].InitAsDescriptorTable(1, &srvTable);
    rootParameters[2].InitAsDescriptorTable(1, &srvInstanceTable); // t1 연결!
   

    // 텍스처를 픽셀에 입힐 때 점으로 찍을지(Point), 부드럽게 뭉갤지(Linear) 결정하는 '샘플러'를 추가합니다.
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT; // 도트(도트픽셀) 느낌을 내기 위해 Point 필터 사용
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 텍스처가 끝나면 반복(Wrap)
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 16;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    sampler.MinLOD = 0;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0; // s0 레지스터에 꽂힙니다.
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // 픽셀 셰이더에서만 사용

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
    // 파라미터 2개, 샘플러 1개를 등록하여 최종 루트 시그니처를 초기화합니다! 합쳐서 3개
    rootSigDesc.Init(3, rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob);

    if (errorBlob != nullptr)
    {
        OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        return false;
    }

    if (FAILED(hr)) return false;

    hr = mDevice->CreateRootSignature(
        0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)
    );

    return SUCCEEDED(hr);
}


bool D3D12Renderer::BuildPSO() // PSO 구축
{
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    ComPtr<ID3DBlob> errorBlob;

    UINT compileFlags = 0;
#if defined(_DEBUG) 
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = D3DCompileFromFile(L"Source/Shaders/color.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &errorBlob);
    if (FAILED(hr)) { if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer()); return false; }

    hr = D3DCompileFromFile(L"Source/Shaders/color.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &errorBlob);
    if (FAILED(hr)) { if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer()); return false; }

    // --- 수정됨: 입력 레이아웃에 NORMAL(법선) 데이터를 추가했습니다! ---
    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        // 12바이트(위치) + 16바이트(색상) = 28바이트 오프셋부터 법선 벡터가 시작됩니다.
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        // 위치(12) + 색상(16) + 법선(12) = 40바이트 오프셋부터 UV 데이터(float2, 8바이트)가 시작됨을 알려줍니다!
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = { reinterpret_cast<BYTE*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<BYTE*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
   
    // [  1] 유리창처럼 뒷면도 보이게 하기 위해 뒷면 가리기(Culling)를 끕니다! 
    // 반투명한 물체는 뒷면도 비쳐 보여야 현실감이 납니다.
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // 기본값인 BACK에서 NONE으로 변경
    // [  2] 반투명 효과의 핵심! 블렌딩 상태(BlendState)를 설정합니다. 
    D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // 기본 블렌드 구조체를 가져옵니다.

    // 첫 번째 렌더 타겟(우리의 도화지)에 대한 블렌딩 옵션을 활성화합니다.
    blendDesc.RenderTarget[0].BlendEnable = TRUE;

    // 이 물체의 픽셀(Source)이 가진 알파(투명도) 비율만큼 색상을 사용하겠다고 설정합니다. (SrcAlpha)
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;

    // 이미 도화지에 그려진 픽셀(Destination)은 (1 - Source의 알파) 비율만큼 섞겠다고 설정합니다. (InvSrcAlpha)
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

    // 두 색상을 섞을 때 덧셈(+) 연산을 사용합니다.
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

    // 알파 채널(투명도 자체)을 섞는 방식입니다. (여기서는 물체의 투명도(1)와 배경의 투명도(0)를 더합니다)
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

    // 설정한 블렌드 상태를 파이프라인에 적용합니다!
    psoDesc.BlendState = blendDesc;





    // 깊이 버퍼(Z-Buffer) 검사를 수행하도록 활성화합니다.
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // Z값을 기록하도록 허용합니다.
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // 기존에 그려진 픽셀보다 '거리가 가까운(값이 작은)' 경우에만 덮어씁니다!
    psoDesc.DepthStencilState.StencilEnable = FALSE; // 스텐실은 아직 안 씁니다.
    // ----------------------------------------------
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    // 우리가 만든 깊이 버퍼의 포맷과 똑같이 파이프라인에 알려줍니다.
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.SampleDesc.Count = 1;

    hr = mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO));
    return SUCCEEDED(hr);
}


//   [구조 분화] 임시 도화지 3장을 읽기 위한 뷰(SRV)를 공용 서랍장에 등록합니다.  
bool D3D12Renderer::BuildConstantBuffers()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    // 칸 개수를 6개로 늘립니다! (Pass, 바닥텍스처, 명부배열, 도화지0, 도화지1, 도화지2)
    cbvHeapDesc.NumDescriptors = 6;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    mDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap));

    //상수버퍼 사이즈 계산
    UINT passCBByteSize = CalcConstantBufferByteSize(sizeof(PassConstants));

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC passBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(passCBByteSize);

    mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &passBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mPassCB));
    mPassCB->Map(0, nullptr, reinterpret_cast<void**>(&mMappedPassCB));

    D3D12_CONSTANT_BUFFER_VIEW_DESC passCbvDesc;
    passCbvDesc.BufferLocation = mPassCB->GetGPUVirtualAddress();
    passCbvDesc.SizeInBytes = passCBByteSize;

    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
    mDevice->CreateConstantBufferView(&passCbvDesc, hDescriptor);

    hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);

    D3D12_SHADER_RESOURCE_VIEW_DESC texSrvDesc = {};
    texSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    texSrvDesc.Format = mTexture->GetDesc().Format;
    texSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    texSrvDesc.Texture2D.MostDetailedMip = 0;
    texSrvDesc.Texture2D.MipLevels = mTexture->GetDesc().MipLevels;
    texSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    mDevice->CreateShaderResourceView(mTexture.Get(), &texSrvDesc, hDescriptor);

    hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);

    UINT instanceBufferSize = sizeof(InstanceData) * NumInstances;
    CD3DX12_RESOURCE_DESC instBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(instanceBufferSize);

    mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &instBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mInstanceBuffer));
    mInstanceBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedInstanceData));

    D3D12_SHADER_RESOURCE_VIEW_DESC instSrvDesc = {};
    instSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    instSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    instSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    instSrvDesc.Buffer.FirstElement = 0;
    instSrvDesc.Buffer.NumElements = NumInstances;
    instSrvDesc.Buffer.StructureByteStride = sizeof(InstanceData);
    instSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    mDevice->CreateShaderResourceView(mInstanceBuffer.Get(), &instSrvDesc, hDescriptor);

    // 임시 도화지 3장의 뷰(SRV)를 연속해서 서랍장에 등록합니다. (인덱스 3, 4, 5번)
    for (int i = 0; i < OffscreenBufferCount; ++i)
    {
        hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
        D3D12_SHADER_RESOURCE_VIEW_DESC offscreenSrvDesc = {};
        offscreenSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        offscreenSrvDesc.Format = mOffscreenTex[i]->GetDesc().Format;
        offscreenSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        offscreenSrvDesc.Texture2D.MostDetailedMip = 0;
        offscreenSrvDesc.Texture2D.MipLevels = 1;
        offscreenSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        mDevice->CreateShaderResourceView(mOffscreenTex[i].Get(), &offscreenSrvDesc, hDescriptor);
    }

    return true;
}

// CPU에서 가상의 '체크무늬 텍스처'를 만들어 GPU로 올리는 완전히 새로운 함수입니다.
bool D3D12Renderer::BuildTexture()
{
    const UINT texWidth = 256;  // 텍스처 가로 크기
    const UINT texHeight = 256; // 텍스처 세로 크기
    const UINT texPixelSize = 4; // 1픽셀당 4바이트 (R, G, B, A)

    // 1. GPU 메모리에 이미지를 담을 '텍스처용 도화지(Default Heap)'를 만듭니다.
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = texWidth;
    texDesc.Height = texHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    mDevice->CreateCommittedResource(
        &defaultHeap, D3D12_HEAP_FLAG_NONE, &texDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, // 데이터를 받을 준비(COPY_DEST) 상태로 둡니다.
        nullptr, IID_PPV_ARGS(&mTexture));

    // 2. CPU 데이터를 GPU로 전달하기 위한 중간 다리(Upload Buffer)를 만듭니다.
    const UINT64 uploadBufferSize = texWidth * texHeight * texPixelSize;
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    mDevice->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mTextureUploadBuffer));

    // 3. CPU에서 수학적으로 예쁜 "체크무늬(Checkerboard)" 배열을 만듭니다.
    std::vector<uint8_t> textureData(uploadBufferSize);
    for (UINT y = 0; y < texHeight; y++)
    {
        for (UINT x = 0; x < texWidth; x++)
        {
            // 32픽셀마다 색이 반전되는 체크무늬 알고리즘입니다.
            bool isWhite = ((x / 32) % 2) == ((y / 32) % 2);
            uint8_t color = isWhite ? 255 : 50; // 흰색(255)과 짙은 회색(50) 교차

            UINT index = (y * texWidth + x) * texPixelSize;
            textureData[index + 0] = color;     // R
            textureData[index + 1] = color;     // G
            textureData[index + 2] = 255;       // B (푸른빛을 살짝 섞어서 세련되게 만듭니다)

            //  [  3] 텍스처의 알파(A) 값을 반투명하게 조절합니다! 
             // 흰색 칸은 70% 정도 불투명하게(180), 회색 칸은 20% 정도만 불투명하게(50) 설정해 봅니다.
            textureData[index + 3] = isWhite ? 180 : 50;
        }
    }

    // 4. 업로드 버퍼를 통해 GPU의 텍스처 도화지에 그림을 덮어씌웁니다.
    // (이 작업은 '명령어'를 통해서만 가능하므로, 닫혀있던 커맨드 리스트를 잠깐 엽니다.)
    mCommandAllocator->Reset();
    mCommandList->Reset(mCommandAllocator.Get(), nullptr);

    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = textureData.data();
    subResourceData.RowPitch = texWidth * texPixelSize;
    subResourceData.SlicePitch = subResourceData.RowPitch * texHeight;

    // d3dx12.h의 마법 같은 함수! 배열 데이터를 GPU 텍스처에 완벽하게 복사해줍니다.
    UpdateSubresources(mCommandList.Get(), mTexture.Get(), mTextureUploadBuffer.Get(), 0, 0, 1, &subResourceData);

    // 5. 복사가 끝난 텍스처를 "셰이더가 읽을 수 있는 상태(PIXEL_SHADER_RESOURCE)"로 배리어 전환합니다.
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    mCommandList->ResourceBarrier(1, &barrier);

    // 커맨드를 제출하고 완료될 때까지 확실하게 기다립니다.
    mCommandList->Close();
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, cmdsLists);
    FlushCommandQueue();

    return true;
}

bool D3D12Renderer::BuildOffscreenRenderTargets() //   임시 도화지 3장을 생성합니다.  
{
    D3D12_RESOURCE_DESC texDesc; // 스펙 구조체입니다.
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = mClientWidth;
    texDesc.Height = mClientHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; // 도화지 허가입니다.

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    optClear.Color[0] = 0.39f; optClear.Color[1] = 0.58f; optClear.Color[2] = 0.93f; optClear.Color[3] = 1.0f;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT); // 고속 힙입니다.

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = OffscreenBufferCount; // 서랍장을 3칸으로 만듭니다!
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mOffscreenRtvHeap)); // 서랍장 객체입니다.

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mOffscreenRtvHeap->GetCPUDescriptorHandleForHeapStart()); // 첫 칸 주소입니다.

    for (int i = 0; i < OffscreenBufferCount; ++i) // 루프를 돌며 3장의 도화지를 생성합니다.
    {
        HRESULT hr = mDevice->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            &optClear, IID_PPV_ARGS(&mOffscreenTex[i])); // 메모리 할당입니다.
        if (FAILED(hr)) return false;

        mDevice->CreateRenderTargetView(mOffscreenTex[i].Get(), nullptr, rtvHandle); // 안경 결합입니다.
        rtvHandle.Offset(1, mRtvDescriptorSize); // 포인터 전진입니다.
    }
    return true; // 성공입니다.
}


//   [구조 분화] 역할에 맞게 계약서를 2가지로 쪼개고, 파이프라인 4개를 생성합니다!  
bool D3D12Renderer::BuildPostProcessPipelines()
{
    // [계약서 1] 싱글 계약서 (텍스처 1개만 입력 - 추출, 가로블러, 세로블러 용)
    CD3DX12_DESCRIPTOR_RANGE srvTableSingle;
    srvTableSingle.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    CD3DX12_ROOT_PARAMETER rootParamSingle[1];
    rootParamSingle[0].InitAsDescriptorTable(1, &srvTableSingle);

    // [계약서 2] 더블 계약서 (텍스처 2개를 동시 입력 - 최종 합성 용)
    CD3DX12_DESCRIPTOR_RANGE srvTableDouble;
    srvTableDouble.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);
    CD3DX12_ROOT_PARAMETER rootParamDouble[1];
    rootParamDouble[0].InitAsDescriptorTable(1, &srvTableDouble);

    // 공통 뭉개기 샘플러
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    sampler.ShaderRegister = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // 싱글 계약서 생성
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDescSingle;
    rootSigDescSingle.Init(1, rootParamSingle, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_NONE);
    ComPtr<ID3DBlob> serializedRootSigSingle, errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDescSingle, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSigSingle, &errorBlob);
    if (FAILED(hr)) return false;
    hr = mDevice->CreateRootSignature(0, serializedRootSigSingle->GetBufferPointer(), serializedRootSigSingle->GetBufferSize(), IID_PPV_ARGS(&mPostRootSigSingle));

    // 더블 계약서 생성
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDescDouble;
    rootSigDescDouble.Init(1, rootParamDouble, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_NONE);
    ComPtr<ID3DBlob> serializedRootSigDouble;
    hr = D3D12SerializeRootSignature(&rootSigDescDouble, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSigDouble, &errorBlob);
    if (FAILED(hr)) return false;
    hr = mDevice->CreateRootSignature(0, serializedRootSigDouble->GetBufferPointer(), serializedRootSigDouble->GetBufferSize(), IID_PPV_ARGS(&mPostRootSigDouble));

    // 개별 셰이더 컴파일
    ComPtr<ID3DBlob> vsQuad, psBrightPass, psBlurH, psBlurV, psComposite;
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    hr = D3DCompileFromFile(L"Source/Shaders/ScreenQuadVS.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vsQuad, &errorBlob);
    if (FAILED(hr)) return false;
    hr = D3DCompileFromFile(L"Source/Shaders/PP_BrightPass.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &psBrightPass, &errorBlob);
    if (FAILED(hr)) return false;
    hr = D3DCompileFromFile(L"Source/Shaders/PP_BlurH.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &psBlurH, &errorBlob);
    if (FAILED(hr)) return false;
    hr = D3DCompileFromFile(L"Source/Shaders/PP_BlurV.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &psBlurV, &errorBlob);
    if (FAILED(hr)) return false;
    hr = D3DCompileFromFile(L"Source/Shaders/PP_Composite.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &psComposite, &errorBlob);
    if (FAILED(hr)) return false;

    // 공통 파이프라인 스펙
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.pRootSignature = mPostRootSigSingle.Get(); // 기본값: 싱글 계약서
    psoDesc.VS = { reinterpret_cast<BYTE*>(vsQuad->GetBufferPointer()), vsQuad->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    // 1. 추출용 PSO 조립
    psoDesc.PS = { reinterpret_cast<BYTE*>(psBrightPass->GetBufferPointer()), psBrightPass->GetBufferSize() };
    hr = mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPsoBrightPass));

    // 2. 가로 블러용 PSO 조립
    psoDesc.PS = { reinterpret_cast<BYTE*>(psBlurH->GetBufferPointer()), psBlurH->GetBufferSize() };
    hr = mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPsoBlurH));

    // 3. 세로 블러용 PSO 조립
    psoDesc.PS = { reinterpret_cast<BYTE*>(psBlurV->GetBufferPointer()), psBlurV->GetBufferSize() };
    hr = mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPsoBlurV));

    // 4. 합성용 PSO 조립 (계약서를 더블로 교체!)
    psoDesc.pRootSignature = mPostRootSigDouble.Get();
    psoDesc.PS = { reinterpret_cast<BYTE*>(psComposite->GetBufferPointer()), psComposite->GetBufferSize() };
    hr = mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPsoComposite));

    return SUCCEEDED(hr);
}

//   =========================================================================  
//   [구조 분화] 3D 오브젝트 렌더링을 전담하는 함수입니다.  
void D3D12Renderer::RenderScene()
{
    // 대상 도화지를 '0번 임시 도화지(Ping)'로 설정합니다.
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mOffscreenTex[0].Get(),
        D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE offscreenRtv(mOffscreenRtvHeap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(mDsvHeap->GetCPUDescriptorHandleForHeapStart());

    const float clearColor[] = { 0.39f, 0.58f, 0.93f, 1.0f };
    mCommandList->ClearRenderTargetView(offscreenRtv, clearColor, 0, nullptr);
    mCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    mCommandList->OMSetRenderTargets(1, &offscreenRtv, FALSE, &dsvHandle);
    mCommandList->RSSetViewports(1, &mViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    CD3DX12_GPU_DESCRIPTOR_HANDLE heapStart(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
    mCommandList->SetGraphicsRootDescriptorTable(0, CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStart, 0, mCbvSrvUavDescriptorSize));
    mCommandList->SetGraphicsRootDescriptorTable(1, CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStart, 1, mCbvSrvUavDescriptorSize));
    mCommandList->SetGraphicsRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStart, 2, mCbvSrvUavDescriptorSize));

    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


    //  렌더러가 들고 있던 뷰 변수 대신, 메시 객체에게 뷰를 꺼내 달라고 요청합니다. 
    D3D12_VERTEX_BUFFER_VIEW vbv = mDefaultBoxMesh->GetVertexBufferView(); // 메시에서 정점 뷰 획득
    mCommandList->IASetVertexBuffers(0, 1, &vbv); // 파이프라인 장착

    D3D12_INDEX_BUFFER_VIEW ibv = mDefaultBoxMesh->GetIndexBufferView(); // 메시에서 인덱스 뷰 획득
    mCommandList->IASetIndexBuffer(&ibv); // 파이프라인 장착

    //  무조건 100개를 그리는 게 아니라, Update에서 걸러낸 '메시 컴포넌트를 가진 진짜 수량(mInstanceCountToDraw)'만큼만 그립니다! 
    if (mInstanceCountToDraw > 0) // 그릴 녀석이 1명이라도 존재할 때만 드로우 콜 발사
    {
        mCommandList->DrawIndexedInstanced(mDefaultBoxMesh->GetIndexCount(), mInstanceCountToDraw, 0, 0, 0); // 그리기!
    }


    // 렌더링이 끝난 도화지는 다시 재료(READ)로 쓸 수 있게 전환합니다.
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mOffscreenTex[0].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
    mCommandList->ResourceBarrier(1, &barrier);
}

//   [19단계 핵심] 상용 엔진의 블룸 아키텍처: 가우시안 핑퐁 렌더링  
void D3D12Renderer::RenderPostProcess()
{
    // 공통 풀스크린 트라이앵글 세팅 (필터 렌더링용)
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 삼각형 지시입니다.
    mCommandList->IASetVertexBuffers(0, 0, nullptr); // 버텍스 꼼수 적용입니다.
    mCommandList->IASetIndexBuffer(nullptr); // 인덱스 꼼수 적용입니다.

    CD3DX12_GPU_DESCRIPTOR_HANDLE heapStart(mCbvHeap->GetGPUDescriptorHandleForHeapStart()); // 서랍장 SRV 구역 시작 주소입니다.
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvStart(mOffscreenRtvHeap->GetCPUDescriptorHandleForHeapStart()); // 서랍장 RTV 구역 시작 주소입니다.

    // ========================================================================
    // --- 패스 1: 밝기 추출 (Bright Pass) ---
    // [0번 원본 도화지] -> (밝기 추출 셰이더) -> [1번 도화지]
    // ========================================================================
    {
        // 텍스처 1개만 입력받는 싱글용 계약서를 장착합니다.
        mCommandList->SetGraphicsRootSignature(mPostRootSigSingle.Get());

        // 1번 도화지를 그리기 상태로 바꿉니다.
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            mOffscreenTex[1].Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
        mCommandList->ResourceBarrier(1, &barrier);

        // 1번 도화지 RTV 주소 연결 (깊이 버퍼는 끔)
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv1(rtvStart, 1, mRtvDescriptorSize);
        mCommandList->OMSetRenderTargets(1, &rtv1, FALSE, nullptr);

        // 밝기 추출 공장 라인 장착
        mCommandList->SetPipelineState(mPsoBrightPass.Get());

        // 서랍장의 3번 인덱스(0번 원본 텍스처 SRV)를 t0에 꽂습니다.
        mCommandList->SetGraphicsRootDescriptorTable(0, CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStart, 3, mCbvSrvUavDescriptorSize));

        mCommandList->DrawInstanced(3, 1, 0, 0); // 화면 전체에 그립니다.

        // 1번 도화지 완료 후 READ 상태로 복구
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            mOffscreenTex[1].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
        mCommandList->ResourceBarrier(1, &barrier);
    }

    // ========================================================================
    // --- 패스 2: 가로 블러 (Horizontal Blur) ---
    // [1번 밝기 추출 도화지] -> (가로 블러 셰이더) -> [2번 도화지]
    // ========================================================================
    {
        // 싱글 계약서를 그대로 씁니다.

        // 2번 도화지를 그리기 상태로 바꿉니다.
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            mOffscreenTex[2].Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
        mCommandList->ResourceBarrier(1, &barrier);

        // 2번 도화지 RTV 주소 연결
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv2(rtvStart, 2, mRtvDescriptorSize);
        mCommandList->OMSetRenderTargets(1, &rtv2, FALSE, nullptr);

        // 가로 블러 공장 라인 장착
        mCommandList->SetPipelineState(mPsoBlurH.Get());

        // 서랍장의 4번 인덱스(1번 밝기 도화지 SRV)를 t0에 꽂습니다.
        mCommandList->SetGraphicsRootDescriptorTable(0, CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStart, 4, mCbvSrvUavDescriptorSize));

        mCommandList->DrawInstanced(3, 1, 0, 0); // 화면 전체에 그립니다.

        // 2번 도화지 완료 후 READ 상태로 복구
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            mOffscreenTex[2].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
        mCommandList->ResourceBarrier(1, &barrier);
    }

    // ========================================================================
    // --- 패스 3: 세로 블러 (Vertical Blur) ---
    // [2번 가로 블러 도화지] -> (세로 블러 셰이더) -> [1번 도화지 (재사용)]
    // ========================================================================
    {
        // 1번 도화지를 덮어쓰기 위해 다시 그리기 상태로 바꿉니다. (진정한 핑퐁의 묘미!)
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            mOffscreenTex[1].Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
        mCommandList->ResourceBarrier(1, &barrier);

        // 1번 도화지 RTV 주소 재연결
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv1(rtvStart, 1, mRtvDescriptorSize);
        mCommandList->OMSetRenderTargets(1, &rtv1, FALSE, nullptr);

        // 세로 블러 공장 라인 장착
        mCommandList->SetPipelineState(mPsoBlurV.Get());

        // 서랍장의 5번 인덱스(2번 가로 블러 도화지 SRV)를 t0에 꽂습니다.
        mCommandList->SetGraphicsRootDescriptorTable(0, CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStart, 5, mCbvSrvUavDescriptorSize));

        mCommandList->DrawInstanced(3, 1, 0, 0); // 화면 전체에 그립니다.

        // 1번 도화지(최종 완성된 빛 무리)를 READ 상태로 복구
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            mOffscreenTex[1].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
        mCommandList->ResourceBarrier(1, &barrier);
    }

    // ========================================================================
    // --- 패스 4: 최종 합성 (Composite) ---
    // [0번 원본] + [1번 최종 빛 무리] -> (합성 셰이더) -> [모니터 백버퍼]
    // ========================================================================
    {
        // 텍스처를 2개 입력받아야 하므로 더블용 계약서로 전격 교체합니다!
        mCommandList->SetGraphicsRootSignature(mPostRootSigDouble.Get());

        // 모니터 백버퍼를 그리기 상태로 전환합니다.
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            mSwapChainBuffer[mCurrBackBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        mCommandList->ResourceBarrier(1, &barrier);

        // 모니터 RTV 주소 연결
        CD3DX12_CPU_DESCRIPTOR_HANDLE finalRtv(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, mRtvDescriptorSize);
        mCommandList->OMSetRenderTargets(1, &finalRtv, FALSE, nullptr);

        // 합성 공장 라인 장착
        mCommandList->SetPipelineState(mPsoComposite.Get());

        // ★ 핵심: 서랍장의 3번 인덱스(0번 원본 SRV)를 꽂으면, 계약서 규약에 의해 그 다음 칸인 4번 인덱스(1번 최종 빛 무리 SRV)까지 자동으로 t0, t1에 쏙 빨려 들어갑니다!
        mCommandList->SetGraphicsRootDescriptorTable(0, CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStart, 3, mCbvSrvUavDescriptorSize));

        mCommandList->DrawInstanced(3, 1, 0, 0); // 화면 전체 합성 그리기

        // 모니터 출력을 위해 PRESENT 상태로 복구합니다.
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            mSwapChainBuffer[mCurrBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        mCommandList->ResourceBarrier(1, &barrier);
    }
}
