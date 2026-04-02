#include "D3D12Renderer.h" // 헤더를 포함합니다.
#include <d3dcompiler.h> // 셰이더 컴파일을 위한 헤더를 포함합니다.
#include <vector> // C++의 동적 배열인 std::vector를 사용하기 위해 포함합니다.
#include <algorithm> 
#include <map> //  재질별 그룹화를 위해 map 자료구조를 추가합니다. 



#include "Graphics/Resources/Mesh.h" // 분리된 메시 구조체를 사용하기 위해 포함합니다.
// 렌더러가 씬과 액터의 정보를 파악할 수 있도록 방금 만든 프레임워크 헤더 파일들을 참조합니다. 

#include "Framework/Core/Scene.h" // 렌더러가 화면에 그릴 타겟 씬(맵)을 파악하기 위함입니다.
#include "Framework/Core/Actor.h" // 씬 안에 들어있는 액터들의 위치(Transform)를 빼오기 위함입니다.
#include "Framework/Components/MeshComponent.h" //  액터가 메시 부품을 달고 있는지 검사하기 위해 포함합니다. 

#include "Framework/Components/CameraComponent.h" //

// 렌더러가 조명 부품을 인지할 수 있도록 헤더를 포함합니다. 
#include "Framework/Components/LightComponent.h" 

//  렌더러가 텍스처와 머티리얼을 생성하고 관리하기 위해 새 헤더들을 불러옵니다. 
#include "Graphics/Resources/Texture.h" // 분리된 메시 구조체를 사용하기 위해 포함합니다.
#include "Graphics/Resources/Material.h" // 분리된 메시 구조체를 사용하기 위해 포함합니다.

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


    //  고해상도 그림자를 위해 태양의 뷰포트는 2048x2048 사이즈로 별도로 세팅합니다! 
    mShadowViewport.TopLeftX = 0.0f; // 그림자 뷰포트 X
    mShadowViewport.TopLeftY = 0.0f; // 그림자 뷰포트 Y
    mShadowViewport.Width = 2048.0f; // 가로 2048 픽셀 (그림자 퀄리티를 결정함)
    mShadowViewport.Height = 2048.0f; // 세로 2048 픽셀
    mShadowViewport.MinDepth = 0.0f; // 최소 깊이
    mShadowViewport.MaxDepth = 1.0f; // 최대 깊이
    mShadowScissorRect = { 0, 0, 2048, 2048 }; // 그림자용 자르기 영역
    //  ========================================================================= 



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
    //  그림자를 그릴 거대한 2048x2048 깊이 텍스처(Shadow Map) 자원을 생성합니다. 
    if (!BuildShadowMap()) return false;

    // ---  [24단계 핵심 추가점: 외부 이미지 파일(PNG) 파싱 및 텍스처 로딩] ---
    mCommandAllocator->Reset(); // 텍스처 GPU 복사를 위해 임시로 명령 할당자를 엽니다.
    mCommandList->Reset(mCommandAllocator.Get(), nullptr); // 명령서를 엽니다.

    mDefaultTexture = std::make_shared<Texture>(); // 텍스처 객체를 메모리에 올립니다.

    // 방금 만든 LoadFromFile 함수를 호출하여 외부 폴더의 "box.png" 이미지 파일을 읽어오라고 지시합니다!
    if (!mDefaultTexture->LoadFromFile(L"Resources/Textures/Test.png", mDevice.Get(), mCommandList.Get()))
    { // 조건문 시작: 만약 파일이 없거나 경로가 틀려서 불러오기에 실패했다면
        OutputDebugStringA("Failed to load Texture file. Falling back to Checkerboard.\n"); // 디버그 창에 실패를 알립니다.
        // 엔진이 뻗지 않도록(Fallback), 우리가 C++로 짰던 기존 체크무늬 패턴을 생성해서 대처합니다.
        mDefaultTexture->CreateCheckerboard(mDevice.Get(), mCommandList.Get());
    } // 조건문 끝

     // 아무 노멀맵을 주지 않으면, 엔진이 자동으로 "파란색 평면" 기본 가짜 노멀맵을 씌워줍니다! 
    mDefaultNormalMap = std::make_shared<Texture>();
    mDefaultNormalMap->CreateFlatNormalMap(mDevice.Get(), mCommandList.Get());


    // 명령서를 닫고 우체통에 넣어 즉시 실행시킵니다.
    mCommandList->Close();
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, cmdsLists);

    // 텍스처 복사가 완전히 끝날 때까지 펜스 동기화로 CPU를 잠시 대기시킵니다.
    FlushCommandQueue();

    // 3. 머티리얼 객체를 메모리에 생성합니다.
    mDefaultMaterial = std::make_shared<Material>();
    // 4. 이 머티리얼에 방금 구워낸 텍스처를 쏙 끼워 넣어 완성합니다!
    mDefaultMaterial->DiffuseMap = mDefaultTexture;





    // --- 엔진 파이프라인 및 데이터 초기화 핵심 ---
    if (!BuildRootSignature()) return false; // 1. 루트 시그니처 세팅
    if (!BuildPSO()) return false;           // 2. 파이프라인 상태 객체 세팅
    //  [삭제됨] if (!BuildTexture()) return false; 호출 구문이 사라졌습니다! 

    mDefaultBoxMesh = std::make_shared<Mesh>(); // 렌더러 소유의 기본 박스 메시를 동적 생성합니다.
    //  새로 만든 LoadFromOBJ를 호출하여 외부 3D 모델 파일을 로드 시도합니다. 
   // 만약 "Resources/Models/model.obj" 파일이 존재하지 않는다면 함수가 false를 반환합니다.
    //if (!mDefaultBoxMesh->LoadFromOBJ("Resources/Models/model.obj", mDevice.Get()))
    //{ // 파일이 없을 경우 (Fallback)
    //    OutputDebugStringA("Failed to load OBJ file. Falling back to default Box.\n"); // 디버그 창에 실패를 알립니다.
    //    mDefaultBoxMesh->CreateBox(mDevice.Get()); // 파일이 없으므로 임시로 기존의 정육면체 큐브 데이터를 생성합니다.
    //} // 조건문 끝
    mDefaultBoxMesh->CreateSphere(mDevice.Get(), 0.5f, 30, 30);
    //  ========================================================================= 
    //   [구조 분화] 변경된 초기화 함수들을 호출합니다.  
    if (!BuildOffscreenRenderTargets()) return false;
    if (!BuildConstantBuffers()) return false;
    if (!BuildPostProcessPipelines()) return false;

    // 초기화 맨 마지막에 UI 공장 라인 조립 함수를 호출합니다! 
    if (!BuildUIPipeline()) return false;

    return true; // 초기화 완벽히 성공!
} // Initialize 함수의 끝

// --- Update 함수 구현 ---
void D3D12Renderer::Update(float deltaTime, Scene* scene, CameraComponent* camera)
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


    //  [ ] C++ 코드는 이 두 줄만 추가되면 완벽합니다! 
    passConstants.TotalTime = totalTime; // 현재 흐른 누적 시간을 셰이더에게 전달합니다. (UV를 움직일 때 사용)
    // 렌더러가 직접 들고 있던 위치 대신, 카메라 객체에게 "너 지금 3D 위치가 어디야?" 하고 물어서 위치값을 가져와 광택 연산용으로 포장합니다. 
    passConstants.EyePosW = camera->GetPosition();

    //   [핵심 로직] 더 이상 렌더러가 수학 공식을 쓰지 않습니다! 
   // 씬을 뒤져서 'LightComponent'를 단 녀석을 찾아 데이터를 그냥 받아오기만 합니다.  
    XMVECTOR lightDir = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f); // 만약 태양이 없으면 기본 빛 방향은 아래입니다.
    passConstants.LightColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // 기본 색상은 흰색입니다.

    //  매 프레임 조명을 수집하기 전, 활성화된 조명 개수를 0으로 리셋합니다. 
    passConstants.ActivePointLightCount = 0;

    const auto& actors = scene->GetActors(); // 씬 명부를 가져옵니다.
    for (auto& actor : actors)
    { // 루프 시작
        auto lightComp = actor->GetComponent<LightComponent>(); // 액터 중에 조명 부품을 단 녀석이 있는지 검사합니다.
        if (lightComp)
        { // 만약 조명 부품을 찾았다면!
             //  조명의 타입(방향광 vs 점광원)을 검사하여 데이터를 다르게 챙깁니다! 
            if (lightComp->Type == ELightType::Directional)
            {
            // 이 액터의 현재 회전각을 추출하여 3D 회전 행렬을 만듭니다.
            XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(actor->GetTransform()->Rotation.x, actor->GetTransform()->Rotation.y, actor->GetTransform()->Rotation.z);

            // 회전 행렬에서 정면(Forward) 방향 화살표를 쏙 뽑아옵니다. 이것이 태양빛의 방향입니다!
            lightDir = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotMat);
            lightDir = XMVector3Normalize(lightDir); // 화살표 길이를 1로 맞춥니다.

            // 부품에 기록된 최신 밝기 색상을 그대로 가져와 GPU에 쏠 택배 상자에 담습니다.
            passConstants.LightColor = lightComp->LightColor;
            }
            //   만약 빛의 타입이 점광원(Point Light)이라면 배열에 수집합니다!  
            else if (lightComp->Type == ELightType::Point)
            {
                //   현재 배열에 들어간 점광원이 최대 제한 개수(4개)보다 적을 때만 수집합니다. (메모리 초과 방어)  
                if (passConstants.ActivePointLightCount < MAX_POINT_LIGHTS)
                {
                    //   현재 이 데이터를 집어넣을 배열의 빈칸 인덱스 번호입니다.  
                    int idx = passConstants.ActivePointLightCount;

                    XMMATRIX worldMat = actor->GetTransform()->GetWorldMatrix(); // 부모-자식 계층이 모두 적용된 최종 월드 행렬을 가져옵니다.
                    XMVECTOR scale, rot, pos; // 행렬을 쪼개어 담을 임시 변수들입니다.
                    XMMatrixDecompose(&scale, &rot, &pos, worldMat); // 월드 행렬을 분해하여 순수한 3D 월드 위치(pos)만 쏙 뽑아냅니다.

                    //   뽑아낸 위치 정보와 조명 설정값들을 배열의 [idx] 번째 칸에 복사해 넣습니다.  
                    XMStoreFloat3(&passConstants.PointLights[idx].PosW, pos);
                    passConstants.PointLights[idx].Color = lightComp->LightColor;
                    passConstants.PointLights[idx].FalloffStart = lightComp->FalloffStart;
                    passConstants.PointLights[idx].FalloffEnd = lightComp->FalloffEnd;

                    //   유효한 조명을 배열에 넣었으므로 활성 조명 개수를 1 증가시킵니다.  
                    passConstants.ActivePointLightCount++;
                }
            }
            
        } // 조건문 끝
    } // 루프 끝

    XMStoreFloat3(&passConstants.LightDir, lightDir); // 찾아낸 최종 빛 방향을 GPU로 보냅니다.

    // --- 그림자 투영 연산 ---
    XMVECTOR lightPos = -lightDir * 50.0f; // 빛 방향 반대편 50m 위 허공에 태양 카메라를 둡니다.
    XMVECTOR targetPos = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f); // 원점을 바라보게 합니다.

    XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // 태양 카메라의 윗방향은 기본 Y축(하늘)입니다.


    //   [안전 장치 추가] 만약 태양이 머리 꼭대기(Y축과 평행)에 정면으로 떠 있다면 수학 공식(LookAt)이 고장나서 터집니다!
    // 이때는 윗방향을 임시로 Z축(앞)으로 비틀어주어 에러를 방지합니다.  
    if (fabsf(XMVectorGetY(lightDir)) > 0.99f)
        lightUp = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp); // 태양 뷰

    // 태양은 직사광선이므로 멀리 있다고 작게 보이지 않습니다. 따라서 직교 투영(Orthographic) 원근 행렬을 씁니다.
    // 40x40 크기의 빛기둥이 1m부터 100m 앞까지 일직선으로 내리꽂힙니다.
    XMMATRIX lightProj = XMMatrixOrthographicLH(40.0f, 40.0f, 1.0f, 100.0f);

    //   [추가점 4] 셰이더(ShadowVS)가 사용할 태양의 순수 ViewProj 행렬을 C++에서 계산하여 넣어줍니다!  
    XMMATRIX lightViewProj = lightView * lightProj;
    XMStoreFloat4x4(&passConstants.LightViewProj, XMMatrixTranspose(lightViewProj));

    // DX12의 공간 좌표(-1~1)를 텍스처를 읽기 위한 UV 좌표(0~1)로 변환해 주는 마법의 보정 행렬입니다.
    XMMATRIX T(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f
    );

    // 태양의 View * Proj * UV보정 행렬을 곱해 섀도우 맵 전용 "빛의 변환 행렬"을 완성합니다!
    XMMATRIX lightTransform = lightView * lightProj * T;

    // 이 행렬을 상수 버퍼 구조체에 담아 픽셀 셰이더로 던져줍니다.
    XMStoreFloat4x4(&passConstants.LightTransform, XMMatrixTranspose(lightTransform));
    // -------------------------------------------------------------------------------------



    memcpy(mMappedPassCB, &passConstants, sizeof(PassConstants));


    //  최강의 렌더링 최적화: 재질(Material)별로 물체들을 묶어주는 Batch 알고리즘입니다! 
    std::map<Material*, std::vector<InstanceData>> matBatches; // 재질 포인터를 키(Key)로 삼아 물체들을 바구니에 담습니다.

    for (auto& actor : actors)
    {
        auto meshComp = actor->GetComponent<MeshComponent>();
        if (meshComp && meshComp->GetMesh())
        {
            InstanceData instData;
            XMMATRIX finalWorld = actor->GetTransform()->GetWorldMatrix();
            XMStoreFloat4x4(&instData.World, XMMatrixTranspose(finalWorld));

            auto mat = meshComp->GetMaterial();
            if (mat) {
                instData.BaseColor = mat->DiffuseAlbedo;
                instData.Emissive = mat->Emissive;
                matBatches[mat.get()].push_back(instData); // 자기 재질 바구니에 데이터를 쏙 넣습니다.
                // 머티리얼의 물리 수치를 인스턴스 배열로 복사합니다. 
                instData.Roughness = mat->Roughness;
                instData.Metallic = mat->Metallic;
            }
            else {
                instData.BaseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
                instData.Emissive = { 0.0f, 0.0f, 0.0f };
                instData.Roughness = 0.5f;
                instData.Metallic = 0.0f;
                matBatches[nullptr].push_back(instData); // 재질이 없으면 널 포인터 바구니에 넣습니다.
            }
        }
    }






    //  렌더 배칭(Render Batching)의 기초! 모든 액터를 무식하게 다 그리지 않고 필터링합니다. 
    mInstanceCountToDraw = 0; // 이번 프레임에 그려야 할 인스턴스 카운트를 0으로 초기화합니다.

    mRenderBatches.clear(); // 매 프레임 그리기 목록을 갱신합니다.

    // 바구니에 담긴 물체들을 거대한 하나의 배열(GPU 버퍼)로 직렬화(Serialization)합니다.
    for (auto& pair : matBatches)
    {
        RenderBatch batch;
        batch.Mat = pair.first; // 이 바구니의 재질
        batch.StartInstance = mInstanceCountToDraw; // 이 바구니의 물체들이 배열의 몇 번부터 시작하는지
        batch.InstanceCount = static_cast<UINT>(pair.second.size()); // 총 몇 개인지

        for (auto& data : pair.second)
        {
            if (mInstanceCountToDraw < NumInstances)
            {
                mMappedInstanceData[mInstanceCountToDraw++] = data; // GPU 메모리로 복사!
            }
        }
        mRenderBatches.push_back(batch); // 완성된 그룹을 목록에 등록합니다.
    }
    //  --------------------------------------------------------------------------------- 
}

//   [구조 분화] 엔진의 메인 Draw 함수가 구조적으로 매우 깔끔해졌습니다!  
void D3D12Renderer::Draw()
{
    mCommandAllocator->Reset();
    mCommandList->Reset(mCommandAllocator.Get(), mPSO.Get());

    //  모니터에 진짜 세상을 그리기 전에, 먼저 "태양의 시점"에서 그림자 텍스처를 한 번 구워냅니다! 
    RenderShadows();

    // 1. 3D 씬을 임시 도화지(Ping)에 렌더링합니다.
    RenderScene();

    // 2. 임시 도화지를 재료로 삼아 2D 필터(포스트 프로세스 체인)를 적용합니다.
    RenderPostProcess();

    // 3D 씬과 포스트 프로세스가 모두 끝난 '가장 마지막'에 UI를 덧그려 덮어씌웁니다! 
    RenderUI();

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

std::shared_ptr<Texture> D3D12Renderer::LoadTexture(const std::wstring& filepath)
{
    // 로딩을 위해 커맨드 리스트를 잠시 엽니다.
    mCommandAllocator->Reset();
    mCommandList->Reset(mCommandAllocator.Get(), nullptr);

    std::shared_ptr<Texture> tex = std::make_shared<Texture>();

    // 파일 로딩에 성공하면 서랍장에 등록하고, 실패하면 기본 체크무늬를 줍니다.
    if (tex->LoadFromFile(filepath, mDevice.Get(), mCommandList.Get())) {
        RegisterTextureSRV(tex);
    }
    else {
        OutputDebugStringA("Texture load failed, falling back to default.\n");
        tex = mDefaultTexture;
    }

    // 파일 로딩 명령을 제출하고 동기화(완료 대기)합니다.
    mCommandList->Close();
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, cmdsLists);
    FlushCommandQueue();

    return tex;
}
//  외부 이미지를 로드하고 자동으로 GPU 서랍장에 꽂아주는 핵심 함수들 구현부입니다! 
void D3D12Renderer::RegisterTextureSRV(std::shared_ptr<Texture> tex)
{
    if (!tex || !tex->GetResource()) return;
    if (mNextTextureIndex >= 50) return; // 서랍장(50칸)이 꽉 차면 무시합니다.

    // 서랍장의 빈칸(mNextTextureIndex)을 찾아 포인터를 만듭니다.
    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mCbvHeap->GetCPUDescriptorHandleForHeapStart(), mNextTextureIndex, mCbvSrvUavDescriptorSize);

    D3D12_SHADER_RESOURCE_VIEW_DESC texSrvDesc = {};
    texSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    texSrvDesc.Format = tex->GetResource()->GetDesc().Format;
    texSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    texSrvDesc.Texture2D.MostDetailedMip = 0;
    texSrvDesc.Texture2D.MipLevels = 1;
    texSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    // 해당 빈칸에 텍스처 뷰(SRV)를 생성해 꽂아 넣습니다.
    mDevice->CreateShaderResourceView(tex->GetResource().Get(), &texSrvDesc, hDescriptor);

    // 텍스처 본인에게 "너는 서랍장 몇 번 칸에 있어"라고 명찰(Handle)을 달아줍니다. 나중에 꺼내 쓸 때 씁니다!
    tex->SrvGpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart(), mNextTextureIndex, mCbvSrvUavDescriptorSize);
    mNextTextureIndex++; // 다음 텍스처를 위해 빈칸 번호를 1 올립니다.
}
// -------------------------------------------------------------------------------------- 



bool D3D12Renderer::BuildRootSignature()
{
    // 새로운 셰이더 규칙(레지스터)에 맞게 계약서(루트 시그니처)를 완전히 새로 작성합니다! 

    // 1번 조항: 공통(Pass) 상수 버퍼 (b0) - 서랍장 형태
    CD3DX12_DESCRIPTOR_RANGE cbvPassTable;
    cbvPassTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // b0

    CD3DX12_DESCRIPTOR_RANGE srvInstanceTable;
    srvInstanceTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // t1 (Instance Array)

    // 3번 조항: 텍스처 (t0) - 서랍장 형태
    CD3DX12_DESCRIPTOR_RANGE srvTextureTable;
    srvTextureTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0
    // 이제 고유 데이터는 CBV(상수 버퍼)가 아니라 SRV(배열 버퍼)로 들어옵니다! 

    //  셰이더가 섀도우 맵 텍스처를 읽어올 수 있도록 t2 슬롯을 새로 계약서에 파줍니다! 
    CD3DX12_DESCRIPTOR_RANGE srvShadowTable;
    srvShadowTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2); // t2 (그림자 텍스처용)

    // 노멀맵 이미지를 꽂아 넣을 레지스터(t3) 칸을 서랍장에 추가합니다! 
    CD3DX12_DESCRIPTOR_RANGE srvNormalTable;
    srvNormalTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3); // t3 레지스터!

    CD3DX12_ROOT_PARAMETER rootParameters[6] = {}; // 배열의 크기를 5칸으로 정확히 할당하여 메모리 초과 에러를 막습니다.
    rootParameters[0].InitAsDescriptorTable(1, &cbvPassTable); // 0번 파라미터: 공통 상수 버퍼 (b0)를 묶습니다.
    rootParameters[1].InitAsDescriptorTable(1, &srvInstanceTable); // 1번 파라미터: 인스턴스 위치 배열 버퍼 (t1)를 묶습니다.
    rootParameters[2].InitAsDescriptorTable(1, &srvTextureTable); // 2번 파라미터: 표면 색상 텍스처 (t0)를 묶습니다.
    rootParameters[3].InitAsDescriptorTable(1, &srvShadowTable); // 3번 파라미터: 섀도우 맵 깊이 텍스처 (t2)를 묶습니다.

    // 새로 추가된 4번 파라미터: 셰이더의 b1 레지스터 공간에 32비트 상수 1개를 보낸다고 계약합니다.
    rootParameters[4].InitAsConstants(1, 1, 0);

    rootParameters[5].InitAsDescriptorTable(1, &srvNormalTable); //  5번 방(t3)에 노멀맵 계약 추가!

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

    //  섀도우 맵 전용 특수 샘플러(비교 샘플러)입니다! 
    // 텍스처를 읽을 때 자동으로 픽셀 깊이와 그림자 깊이를 비교(Less Equal)해서 0.0(그림자) 또는 1.0(빛)을 반환해 줍니다. 
    D3D12_STATIC_SAMPLER_DESC shadowSampler = {};
    shadowSampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; // 비교(Comparison) 필터링
    shadowSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER; // 섀도우 맵 바깥은 그림자가 안 지도록 경계색 처리
    shadowSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    shadowSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    shadowSampler.MipLODBias = 0;
    shadowSampler.MaxAnisotropy = 16;
    shadowSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // 내 깊이가 그림자보다 작거나 같으면 통과!
    //   [최적화 수정] 섀도우 맵 영역 바깥쪽은 기본적으로 '하얀색(그림자 없음)'으로 취급하도록 세팅합니다!  
    shadowSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE; // OPAQUE_BLACK에서 WHITE로 변경

    shadowSampler.MinLOD = 0;
    shadowSampler.MaxLOD = D3D12_FLOAT32_MAX;
    shadowSampler.ShaderRegister = 1; //  s1 슬롯 할당
    shadowSampler.RegisterSpace = 0;
    shadowSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // 샘플러 배열을 2개로 묶어줍니다.
    D3D12_STATIC_SAMPLER_DESC samplers[2] = { sampler, shadowSampler };




    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc; // DX12 루트 시그니처 스펙 구조체를 선언합니다.
    //  넘겨줄 파라미터가 5개가 되었으므로, Init의 첫 번째 인자도 5로 변경합니다! 
    rootSigDesc.Init(6, rootParameters, 2, samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

    //   [추가점 5] 그림자 전용 버텍스 셰이더의 결과물을 담을 바구니를 하나 더 준비합니다!  
    ComPtr<ID3DBlob> shadowVertexShader;

    ComPtr<ID3DBlob> errorBlob;

    UINT compileFlags = 0;
#if defined(_DEBUG) 
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = D3DCompileFromFile(L"Source/Shaders/color.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &errorBlob);
    if (FAILED(hr)) { if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer()); return false; }

    hr = D3DCompileFromFile(L"Source/Shaders/color.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &errorBlob);
    if (FAILED(hr)) { if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer()); return false; }

    //   [추가점 6] color.hlsl 안에 새로 만든 'ShadowVS' 함수만 따로 컴파일해서 뽑아옵니다!  
    hr = D3DCompileFromFile(L"Source/Shaders/color.hlsl", nullptr, nullptr, "ShadowVS", "vs_5_0", compileFlags, 0, &shadowVertexShader, &errorBlob);
    if (FAILED(hr)) { if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer()); return false; }

    // --- 수정됨: 입력 레이아웃에 NORMAL(법선) 데이터를 추가했습니다! ---
    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        // 12바이트(위치) + 16바이트(색상) = 28바이트 오프셋부터 법선 벡터가 시작됩니다.
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        // 위치(12) + 색상(16) + 법선(12) = 40바이트 오프셋부터 UV 데이터(float2, 8바이트)가 시작됨을 알려줍니다!
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 } //  추가됨
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

    // ---  오직 그림자(깊이)만 계산하는 섀도우 전용 파이프라인(mPsoShadow)을 굽습니다!  ---
    D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc = psoDesc; // 기본 설정을 그대로 복사해 옵니다.

    //   [추가점 7] 섀도우 전용 파이프라인(mPsoShadow)은 일반 VS가 아니라, 방금 컴파일한 '그림자 전용 VS'를 장착합니다!  
    shadowPsoDesc.VS = { reinterpret_cast<BYTE*>(shadowVertexShader->GetBufferPointer()), shadowVertexShader->GetBufferSize() };


    // 핵심 1: 그림자 구울 때는 색상을 안 칠하므로 픽셀 셰이더(PS)를 꺼버립니다. (엄청난 속도 향상)
    shadowPsoDesc.PS = { nullptr, 0 };
    // 핵심 2: RTV(색상 도화지)도 안 쓰므로 꺼버립니다. 오직 DSV(Z-버퍼)만 씁니다.
    shadowPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
    shadowPsoDesc.NumRenderTargets = 0;

    // 핵심 3: Shadow Acne (그림자 표면에 줄무늬가 생기는 그래픽 깨짐 현상)를 방지하기 위해, 
    // 래스터라이저 단계에서 깊이 값(Z)을 아주 살짝 뒤로 밀어버리는 Bias(편향)를 적용합니다.
    shadowPsoDesc.RasterizerState.DepthBias = 100000;
    shadowPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
    shadowPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;

    // 섀도우 전용 파이프라인(mPsoShadow)을 생성합니다!
    mDevice->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&mPsoShadow));
    // ---------------------------------------------------------------------------------------------------

    //  여기서부터 스카이박스 전용 파이프라인(PSO)을 조립합니다! 
    ComPtr<ID3DBlob> skyVS, skyPS;
    D3DCompileFromFile(L"Source/Shaders/sky.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &skyVS, &errorBlob);
    if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    D3DCompileFromFile(L"Source/Shaders/sky.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &skyPS, &errorBlob);
    if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());

    D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = psoDesc;
    skyPsoDesc.VS = { reinterpret_cast<BYTE*>(skyVS->GetBufferPointer()), skyVS->GetBufferSize() };
    skyPsoDesc.PS = { reinterpret_cast<BYTE*>(skyPS->GetBufferPointer()), skyPS->GetBufferSize() };

    // 핵심 트릭 1: 큐브의 '안쪽 면'을 봐야 하므로, 바깥면(Front)을 그리지 않고 날려버립니다!
    skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    // 핵심 트릭 2: 셰이더에서 깊이를 강제로 1.0(가장 멂)으로 고정했으므로, 
    // LESS(작을 때만 그리기)가 아니라 LESS_EQUAL(같거나 작을 때 그리기)로 바꿔줘야 화면에 나타납니다!
    skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    mDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPsoSkybox));
    // 




    return SUCCEEDED(hr);
}


//   [구조 분화] 임시 도화지 3장을 읽기 위한 뷰(SRV)를 공용 서랍장에 등록합니다.  
bool D3D12Renderer::BuildConstantBuffers()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    // 행성마다 여러 개의 텍스처를 꽂을 수 있도록 서랍장 크기를 7칸에서 50칸으로 폭증시킵니다! 
    cbvHeapDesc.NumDescriptors = 50;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    mDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap));

    //   [0번 칸]: 상수 버퍼 (CBV) 세팅  
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

    //   [1번 칸]: 인스턴스 버퍼(큐브 위치 데이터) 세팅 (여기가 텍스처보다 먼저 와야 합니다!!)  
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

    //   [2번 칸]: 텍스처 이미지 세팅  
    hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);

    D3D12_SHADER_RESOURCE_VIEW_DESC texSrvDesc = {};
    texSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    texSrvDesc.Format = mDefaultTexture->GetResource()->GetDesc().Format;
    texSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    texSrvDesc.Texture2D.MostDetailedMip = 0;
    texSrvDesc.Texture2D.MipLevels = 1;
    texSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    mDevice->CreateShaderResourceView(mDefaultTexture->GetResource().Get(), &texSrvDesc, hDescriptor);

    //  기본 텍스처(체크무늬)에게 "너는 2번 서랍장에 있어"라고 명찰을 달아줍니다. 
    mDefaultTexture->SrvGpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart(), 2, mCbvSrvUavDescriptorSize);
    
    //  3: Default Normal Map (가짜 굴곡) 
    hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
    D3D12_SHADER_RESOURCE_VIEW_DESC normSrvDesc = {};
    normSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    normSrvDesc.Format = mDefaultNormalMap->GetResource()->GetDesc().Format;
    normSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    normSrvDesc.Texture2D.MostDetailedMip = 0;
    normSrvDesc.Texture2D.MipLevels = 1;
    mDevice->CreateShaderResourceView(mDefaultNormalMap->GetResource().Get(), &normSrvDesc, hDescriptor);
    mDefaultNormalMap->SrvGpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart(), 3, mCbvSrvUavDescriptorSize);



    // 4, 5, 6: Offscreen Render Targets (포스트 프로세싱)
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

    //   [7번 칸]: 섀도우 맵 뷰 세팅  
    hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);

    D3D12_SHADER_RESOURCE_VIEW_DESC shadowSrvDesc = {};
    shadowSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shadowSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    shadowSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    shadowSrvDesc.Texture2D.MostDetailedMip = 0;
    shadowSrvDesc.Texture2D.MipLevels = 1;
    shadowSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    mDevice->CreateShaderResourceView(mShadowMap.Get(), &shadowSrvDesc, hDescriptor);

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
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(mOffscreenTex[0].Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET); // 배리어
    mCommandList->ResourceBarrier(1, &barrier); // 적용

    CD3DX12_CPU_DESCRIPTOR_HANDLE offscreenRtv(mOffscreenRtvHeap->GetCPUDescriptorHandleForHeapStart()); // 임시 도화지 안경
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(mDsvHeap->GetCPUDescriptorHandleForHeapStart()); // 화면 깊이 안경

    const float clearColor[] = { 0.39f, 0.58f, 0.93f, 1.0f }; // 하늘색 배경
    mCommandList->ClearRenderTargetView(offscreenRtv, clearColor, 0, nullptr); // 도화지 밀기
    mCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr); // 화면 깊이 밀기

    mCommandList->OMSetRenderTargets(1, &offscreenRtv, FALSE, &dsvHandle); // 타겟 장착
    mCommandList->RSSetViewports(1, &mViewport); // 모니터 해상도 뷰포트
    mCommandList->RSSetScissorRects(1, &mScissorRect); // 시저

    ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() }; // 공용 서랍장
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps); // 서랍 묶기

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get()); // 통신 계약서 제출

    // 기본 파이프라인(mPSO)을 장착합니다. (색상과 조명을 전부 칠하는 진짜 그리기 공정)
    mCommandList->SetPipelineState(mPSO.Get());

    CD3DX12_GPU_DESCRIPTOR_HANDLE heapStart(mCbvHeap->GetGPUDescriptorHandleForHeapStart()); // 서랍장 주소
    mCommandList->SetGraphicsRootDescriptorTable(0, CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStart, 0, mCbvSrvUavDescriptorSize)); // 0번 칸: 카메라 정보
    mCommandList->SetGraphicsRootDescriptorTable(1, CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStart, 1, mCbvSrvUavDescriptorSize)); // 1번 칸: 100개 상자 위치
    mCommandList->SetGraphicsRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStart, 2, mCbvSrvUavDescriptorSize)); // 2번 칸: 큐브 표면 텍스처 이미지

    // 방금 RenderShadow() 함수에서 구워낸 '완성된 그림자 텍스처(섀도우 맵)' 뷰를 3번 칸(t2)에 쏙 꽂아줍니다! 
   // 이렇게 하면 셰이더가 이 텍스처를 읽고 픽셀을 까맣게 칠해버립니다.
    mCommandList->SetGraphicsRootDescriptorTable(3, CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStart, 6, mCbvSrvUavDescriptorSize));



    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 삼각형 그리기


    //  렌더러가 들고 있던 뷰 변수 대신, 메시 객체에게 뷰를 꺼내 달라고 요청합니다. 
    D3D12_VERTEX_BUFFER_VIEW vbv = mDefaultBoxMesh->GetVertexBufferView(); // 메시에서 정점 뷰 획득
    mCommandList->IASetVertexBuffers(0, 1, &vbv); // 파이프라인 장착

    D3D12_INDEX_BUFFER_VIEW ibv = mDefaultBoxMesh->GetIndexBufferView(); // 메시에서 인덱스 뷰 획득
    mCommandList->IASetIndexBuffer(&ibv); // 파이프라인 장착
    // 거대한 단 1번의 그리기를 쪼개어, '같은 텍스처를 쓰는 그룹(Batch)' 단위로 반복해서 그립니다! 
       // 성능(Instancing)과 유연성(다중 텍스처)을 모두 잡은 궁극의 최적화입니다.
    for (auto& batch : mRenderBatches)
    {
        // 이 그룹이 사용할 텍스처를 서랍장에서 찾아 파이프라인 2번 칸에 꽂습니다.
        if (batch.Mat && batch.Mat->DiffuseMap) {
            mCommandList->SetGraphicsRootDescriptorTable(2, batch.Mat->DiffuseMap->SrvGpuHandle);
        }
        else {
            mCommandList->SetGraphicsRootDescriptorTable(2, mDefaultTexture->SrvGpuHandle);
        }

        // 노멀맵이 등록되어 있다면 5번 계약 슬롯(t3)에 진짜 노멀맵을 꽂아줍니다! 없으면 가짜 파란 평면을 꽂습니다. 
        if (batch.Mat && batch.Mat->NormalMap) {
            mCommandList->SetGraphicsRootDescriptorTable(5, batch.Mat->NormalMap->SrvGpuHandle);
        }
        else {
            mCommandList->SetGraphicsRootDescriptorTable(5, mDefaultNormalMap->SrvGpuHandle);
        }

        //  셰이더에게 "이번에 그릴 녀석들은 명부의 batch.StartInstance 번호부터 시작해!" 라고 상수로 직접 쏴줍니다. 
        mCommandList->SetGraphicsRoot32BitConstant(4, batch.StartInstance, 0);

        //  DX12는 StartInstanceLocation 인자를 셰이더(SV_InstanceID)에 자동 반영해주지 않습니다!
        // 어차피 우리가 방금 상수로 넘겨서 셰이더 내부에서 더할 것이므로, API 호출 시에는 무조건 0으로 고정합니다. 
        mCommandList->DrawIndexedInstanced(mDefaultBoxMesh->GetIndexCount(), batch.InstanceCount, 0, 0, 0);
    }
    //  --------------------------------------------------------------------------------------------------- 

    //   [수정 지점] 여기서부터 3D 세상 배경이 될 스카이박스를 그립니다!  
    mCommandList->SetPipelineState(mPsoSkybox.Get()); // 스카이박스 전용 셰이더로 교체

    //   [변경 1] 스카이박스 크기를 500.0f배 키워, 카메라 렌즈(Near Z)에 잘리지 않게 만듭니다!  
    XMMATRIX skyWorld = XMMatrixScaling(500.0f, 500.0f, 500.0f) * XMMatrixTranslation(mMappedPassCB->EyePosW.x, mMappedPassCB->EyePosW.y, mMappedPassCB->EyePosW.z);

    //   [변경 2] 0번 인덱스(태양)를 덮어쓰지 않고, 배열의 맨 끝 빈 공간에 스카이박스 데이터를 기록합니다!  
    UINT skyIndex = mInstanceCountToDraw;
    if (skyIndex < NumInstances)
    {
        XMStoreFloat4x4(&mMappedInstanceData[skyIndex].World, XMMatrixTranspose(skyWorld));
        mMappedInstanceData[skyIndex].BaseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        mMappedInstanceData[skyIndex].Emissive = { 0.0f, 0.0f, 0.0f };

        mCommandList->DrawIndexedInstanced(mDefaultBoxMesh->GetIndexCount(), 1, 0, 0, skyIndex);
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


//  섀도우 맵(깊이 텍스처) 전용 리소스를 생성하는 함수 전체 구현부입니다. 
bool D3D12Renderer::BuildShadowMap()
{ // 함수 시작
    D3D12_RESOURCE_DESC texDesc = {}; // 텍스처 스펙을 준비합니다.
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2D 평면 이미지입니다.
    texDesc.Alignment = 0; // 정렬 기본값입니다.
    texDesc.Width = 2048; // 그림자의 퀄리티를 좌우하는 가로 해상도입니다. (크면 선명, 작으면 계단 현상)
    texDesc.Height = 2048; // 세로 해상도입니다.
    texDesc.DepthOrArraySize = 1; // 1장입니다.
    texDesc.MipLevels = 1; // 밉맵 사용 안 합니다.

    //  핵심: 그림자를 기록할 때는 깊이 포맷(DSV)으로 쓰고, 나중에 읽을 때는 이미지 포맷(SRV)으로 써야 합니다.
    // 두 가지 용도로 다 쓸 수 있게 '타입리스(TYPELESS)' 포맷으로 텍스처를 생성합니다! 
    texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

    texDesc.SampleDesc.Count = 1; // 안티앨리어싱 안 씁니다.
    texDesc.SampleDesc.Quality = 0; // 안티앨리어싱 품질입니다.
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // 기본 배치입니다.
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // 이 텍스처에 깊이 기록을 허용합니다.

    D3D12_CLEAR_VALUE optClear; // 그림자 도화지를 싹 밀어버릴 기본값입니다.
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 지울 때는 깊이 전용 포맷을 명시해야 합니다.
    optClear.DepthStencil.Depth = 1.0f; // 가장 먼 거리인 1.0으로 가득 채웁니다.
    optClear.DepthStencil.Stencil = 0; // 스텐실은 0으로 비웁니다.

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT); // 가장 빠른 GPU VRAM에 위치시킵니다.
    // 실제 섀도우 맵 메모리를 할당합니다. 상태는 'GENERIC_READ'로 두어 셰이더가 대기하게 합니다.
    HRESULT hr = mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &optClear, IID_PPV_ARGS(&mShadowMap));
    if (FAILED(hr)) return false; // 생성 실패 시 반환합니다.

    // 이 텍스처에 태양 시점의 깊이를 기록하려면 전용 DSV(깊이) 서랍장이 1칸 필요합니다.
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1; // 1칸짜리 서랍장입니다.
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // DSV 용도입니다.
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // 셰이더 숨김입니다.
    dsvHeapDesc.NodeMask = 0; // 단일 GPU입니다.
    mDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mShadowDsvHeap)); // 전용 서랍장을 만듭니다.

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {}; // DSV 안경 스펙입니다.
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE; // 기본 플래그입니다.
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2D 텍스처입니다.
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 아까 만든 TYPELESS 메모리를 깊이 포맷 안경으로 해석하여 씁니다!
    dsvDesc.Texture2D.MipSlice = 0; // 첫 번째 밉맵입니다.

    // 완성된 DSV 안경을 텍스처에 씌워 아까 만든 1칸짜리 전용 서랍장에 꽂아둡니다.
    mDevice->CreateDepthStencilView(mShadowMap.Get(), &dsvDesc, mShadowDsvHeap->GetCPUDescriptorHandleForHeapStart());

    return true; // 무사히 생성 완료!
} // 섀도우 맵 함수 끝


//   [신규 함수] 플레이어 눈이 아닌 "태양의 시점"으로 오직 물체의 깊이(거리)만 텍스처에 기록하는 그림자 그리기 전담 함수입니다!  
void D3D12Renderer::RenderShadows()
{ // 함수 시작
    // 방금 전까지 읽기 상태였던 섀도우 맵 텍스처를 "깊이(Z버퍼) 기록 모드"로 바꾸라고 차단기(배리어)를 내립니다.
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    mCommandList->ResourceBarrier(1, &barrier); // 명령서에 올립니다.

    // 섀도우 맵 전용 서랍장에서 DSV(깊이) 안경 주소를 꺼내옵니다.
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(mShadowDsvHeap->GetCPUDescriptorHandleForHeapStart());

    // 이전에 기록된 옛날 그림자들을 지우고 도화지를 1.0(가장 멈)으로 깨끗하게 밉니다.
    mCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // 중요: 섀도우 맵은 색상을 칠할 필요가 없으므로 RTV(색상 도화지)는 nullptr로 비워두고, 오직 DSV(깊이 도화지)만 타겟으로 묶습니다!
    mCommandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

    // 모니터 해상도가 아니라 2048x2048 사이즈인 그림자 전용 뷰포트와 자르기 영역을 장착합니다.
    mCommandList->RSSetViewports(1, &mShadowViewport);
    mCommandList->RSSetScissorRects(1, &mShadowScissorRect);

    // 공용 서랍장(행렬 데이터)을 묶어줍니다.
    ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get()); // 통신 계약서를 들이밉니다.

    //   핵심: 일반 렌더링용 공장 라인(mPSO)이 아니라, 색칠 작업을 모두 건너뛰고 깊이만 고속으로 찍어내는 "그림자 전용 공장 라인(mPsoShadow)"을 가동합니다!  
    mCommandList->SetPipelineState(mPsoShadow.Get());

    CD3DX12_GPU_DESCRIPTOR_HANDLE heapStart(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
    mCommandList->SetGraphicsRootDescriptorTable(0, CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStart, 0, mCbvSrvUavDescriptorSize)); // 카메라 행렬 꽂기 (이번엔 태양 시점이 담겨 있음)
    mCommandList->SetGraphicsRootDescriptorTable(1, CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStart, 1, mCbvSrvUavDescriptorSize)); // 100개 큐브 위치 행렬 꽂기
    //   [기존 코드 삭제] (텍스처 이미지는 깊이만 잴 때는 필요 없으므로 2번 칸은 묶지 않습니다.) <-- 이 주석을 지워주세요!  

    //   [추가점] 치명적 버그 수정! 그림자 셰이더가 텍스처를 안 쓰더라도, 계약서(RootSignature)의 4칸은 무조건 다 채워야 GPU가 뻗지 않습니다!  
    // 단, 그림자 맵(t2)은 현재 '쓰기 모드'이므로 '읽기 모드'로 꽂으면 충돌이 납니다. 따라서 빈 2번(t0)과 3번(t2) 칸에 모두 임시 더미로 일반 텍스처를 꽂아 에러를 우회합니다!
    mCommandList->SetGraphicsRootDescriptorTable(2, CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStart, 2, mCbvSrvUavDescriptorSize)); // 2번 칸(t0) 에 일반 텍스처 꽂기
    mCommandList->SetGraphicsRootDescriptorTable(3, CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStart, 2, mCbvSrvUavDescriptorSize)); // 3번 칸(t2) 에도 일반 텍스처 꽂기 (충돌 방지용)

    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 삼각형 연결 세팅

    D3D12_VERTEX_BUFFER_VIEW vbv = mDefaultBoxMesh->GetVertexBufferView(); // 큐브 정점 주입
    mCommandList->IASetVertexBuffers(0, 1, &vbv);
    D3D12_INDEX_BUFFER_VIEW ibv = mDefaultBoxMesh->GetIndexBufferView(); // 큐브 인덱스 주입
    mCommandList->IASetIndexBuffer(&ibv);

    if (mInstanceCountToDraw > 0) // 그려야 할 상자가 있다면
    { // 조건문 시작

           //  그림자는 모든 물체를 '한 번에 통째로(0번부터 끝까지)' 찍어내므로 시작 번호는 0번입니다. 
        mCommandList->SetGraphicsRoot32BitConstant(4, 0, 0);
        // 단 1번의 인스턴싱 드로우 콜로 100개의 상자를 태양 시점 텍스처에 깊이값으로 고속으로 찍어냅니다!
        mCommandList->DrawIndexedInstanced(mDefaultBoxMesh->GetIndexCount(), mInstanceCountToDraw, 0, 0, 0);
    } // 조건문 끝

    // 기록이 끝난 섀도우 맵 텍스처를, 픽셀 셰이더가 "그림자가 졌는지 읽어갈 수 있도록" 읽기 전용 상태로 복구하는 배리어를 내립니다.
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
    mCommandList->ResourceBarrier(1, &barrier); // 명령서 적용
} // 그림자 렌더 함수 끝


//  게임 루프에서 UI를 띄우라고 요청하면, 즉시 그리지 않고 대기열(Queue)에 담아둡니다. 
void D3D12Renderer::DrawUI(std::shared_ptr<Texture> tex, float x, float y, float w, float h)
{ // 함수 시작
    if (!tex) return; // 텍스처가 비어있으면 무시합니다.
    mUIQueue.push_back({ tex, x, y, w, h }); // 전달받은 UI 정보를 배열 맨 뒤에 차곡차곡 쌓아둡니다.
} // 함수 끝

//  UI 전용 계약서와 파이프라인(PSO)을 조립하는 함수입니다. 
bool D3D12Renderer::BuildUIPipeline()
{ // 함수 시작
    // 1. UI용 계약서(루트 시그니처) 작성
    CD3DX12_ROOT_PARAMETER rootParams[2]; // UI는 딱 2칸만 씁니다.

    // 0번 칸: 루트 상수(Root Constants) 6개 할당 (X, Y, W, H, ScreenW, ScreenH)
    rootParams[0].InitAsConstants(6, 0); // 32비트 상수 6개를 b0 레지스터에 꽂겠다고 선언합니다.

    // 1번 칸: UI 이미지 텍스처 1장 할당
    CD3DX12_DESCRIPTOR_RANGE srvTable;
    srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 레지스터
    rootParams[1].InitAsDescriptorTable(1, &srvTable);

    // 샘플러 (텍스처를 부드럽게 읽어옵니다)
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // Linear 필터 사용
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // UI용 루트 시그니처 생성
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
    rootSigDesc.Init(2, rootParams, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature, error;
    D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mUIRootSig));

    // 2. 셰이더 컴파일
    ComPtr<ID3DBlob> vs, ps;
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    D3DCompileFromFile(L"Source/Shaders/ui.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vs, nullptr);
    D3DCompileFromFile(L"Source/Shaders/ui.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &ps, nullptr);

    // 3. UI 전용 파이프라인(PSO) 세팅
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 }; // 정점 버퍼 안 씀! (셰이더에서 자체 생성)
    psoDesc.pRootSignature = mUIRootSig.Get();
    psoDesc.VS = { reinterpret_cast<BYTE*>(vs->GetBufferPointer()), vs->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<BYTE*>(ps->GetBufferPointer()), ps->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // UI는 양면 렌더링

    // [투명도 블렌딩 활성화] 배경과 자연스럽게 섞이도록 세팅
    D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    psoDesc.BlendState = blendDesc;

    psoDesc.DepthStencilState.DepthEnable = FALSE; // UI는 깊이(Z버퍼) 무시하고 무조건 제일 위에 그림!
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPsoUI));

    return true;
} // 함수 끝

//  대기열에 쌓인 UI들을 실제로 화면에 찍어내는 렌더링 함수입니다. 
void D3D12Renderer::RenderUI()
{ // 함수 시작
    if (mUIQueue.empty()) return; // 띄울 UI가 없으면 함수를 즉시 종료합니다.

    // 포스트 프로세싱이 끝나고 'PRESENT(출력 대기)' 상태로 변해있던 백버퍼 도화지를 다시 '그리기(RTV)' 상태로 되돌립니다.
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mSwapChainBuffer[mCurrBackBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCommandList->ResourceBarrier(1, &barrier);

    // 백버퍼 안경(RTV)을 다시 장착합니다. (Z-버퍼는 UI에 필요 없으므로 nullptr 처리)
    CD3DX12_CPU_DESCRIPTOR_HANDLE finalRtv(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, mRtvDescriptorSize);
    mCommandList->OMSetRenderTargets(1, &finalRtv, FALSE, nullptr);

    // UI용 계약서와 파이프라인을 교체 장착합니다.
    mCommandList->SetGraphicsRootSignature(mUIRootSig.Get());
    mCommandList->SetPipelineState(mPsoUI.Get());
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 대기열에 쌓인 모든 UI를 하나씩 꺼내서 그립니다.
    for (const auto& sprite : mUIQueue)
    { // 루프 시작
        // 1. 루트 상수(Root Constants) 세팅
        // X, Y, W, H, ScreenW, ScreenH 순서대로 6개의 값을 배열로 묶습니다.
        float uiData[6] = { sprite.X, sprite.Y, sprite.W, sprite.H, (float)mClientWidth, (float)mClientHeight };

        // 셰이더의 0번 슬롯에 이 6개의 실수 데이터를 초고속으로 직격탄으로 꽂아 넣습니다!
        mCommandList->SetGraphicsRoot32BitConstants(0, 6, uiData, 0);

        // 2. UI 이미지(텍스처) 세팅
        // 셰이더의 1번 슬롯에 텍스처 뷰가 있는 서랍장 위치를 알려줍니다.
        mCommandList->SetGraphicsRootDescriptorTable(1, sprite.Tex->SrvGpuHandle);

        // 3. 그리기 명령! (정점 6개를 이용해서 사각형 1개를 찍어냅니다)
        mCommandList->DrawInstanced(6, 1, 0, 0);
    } // 루프 끝

    mUIQueue.clear(); // 이번 프레임의 UI를 다 그렸으므로 대기열을 싹 비워줍니다.

    // 렌더링이 끝났으니 백버퍼 도화지를 모니터에 출력할 수 있도록 다시 'PRESENT' 상태로 전환합니다.
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mSwapChainBuffer[mCurrBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    mCommandList->ResourceBarrier(1, &barrier);
} // 함수 끝