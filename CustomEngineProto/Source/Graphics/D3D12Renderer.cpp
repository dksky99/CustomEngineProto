#include "D3D12Renderer.h" // 헤더를 포함합니다.
#include <d3dcompiler.h> // 셰이더 컴파일을 위한 헤더를 포함합니다.
#include <vector> // C++의 동적 배열인 std::vector를 사용하기 위해 포함합니다.
#include <algorithm> 


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

    
    // 더 이상 여기서 고정된 View 행렬을 만들지 않습니다. Update 함수에서 매 프레임 실시간으로 만들 것입니다.
    XMStoreFloat4x4(&mView, XMMatrixIdentity()); // 일단 단위 행렬로 초기화해 둡니다.

    // 화면 비율에 맞는 원근감을 만들어내는 투영(Projection) 행렬을 생성합니다. (시야각 45도)
    XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(width) / height, 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, proj); // 멤버 변수에 저장합니다.

    // 10x10 격자 모양으로 총 100개의 큐브 위치(World 행렬)를 미리 쫙 깔아둡니다! 
    for (int i = 0; i < 10; ++i) // 세로(Z축) 10줄
    {
        for (int j = 0; j < 10; ++j) // 가로(X축) 10줄
        {
            int index = i * 10 + j; // 0 ~ 99번 인덱스

            // 큐브 사이의 간격을 2.0 단위로 벌려서 바둑판처럼 배치합니다. 중앙(0,0,0)을 기준으로 퍼지게 만듭니다.
            float xPos = (j - 4.5f) * 2.0f;
            float zPos = (i - 4.5f) * 2.0f;

            // 이동(Translation) 행렬만 만들어서 해당 인덱스에 저장해 둡니다. (아직 회전은 적용 안 함)
            XMMATRIX world = XMMatrixTranslation(xPos, 0.0f, zPos);
            XMStoreFloat4x4(&mWorld[index], world);
        }
    }







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
    if (!BuildGeometry()) return false;      // 3. 기하학(버텍스+인덱스) 데이터 세팅
    //   [14단계 중요 수정] 초기화 순서 변경!  
   // 공용 서랍장(BuildConstantBuffers)을 만들기 전에 임시 도화지(Offscreen)가 먼저 존재해야 합니다.
   // 그래야 서랍장의 4번째 칸에 임시 도화지를 끼워 넣을 수 있기 때문입니다.
    if (!BuildOffscreenRenderTarget()) return false; // 1. 임시 도화지 먼저 생성!
    if (!BuildConstantBuffers()) return false; // 2. 그 다음 서랍장 생성 및 도화지 끼워넣기!

    // 마지막으로 포스트 프로세스 전용 파이프라인(PSO)을 조립합니다!
    if (!BuildPostProcessPipeline()) return false; // 3. 필터 전용 파이프라인 생성!
    //   =========================================================================  


    GetCursorPos(&mLastMousePos);//마우스 델타값을 구하기 위해 시작할 때의 초기 마우스 위치를 한 번 찍어둡니다.

    return true; // 초기화 완벽히 성공!
} // Initialize 함수의 끝

// --- Update 함수 구현 ---
void D3D12Renderer::Update(float deltaTime)
{
    // 1. === 마우스 회전(시선 처리) 로직 ===
    POINT currentMousePos;
    GetCursorPos(&currentMousePos);

    if ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0)
    {
        float dx = XMConvertToRadians(0.25f * static_cast<float>(currentMousePos.x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(currentMousePos.y - mLastMousePos.y));

        mCameraYaw += dx;
        mCameraPitch += dy;

        mCameraPitch = std::clamp(mCameraPitch, -1.5f, 1.5f);
    }
    mLastMousePos = currentMousePos;

    // 2. === 방향 벡터 계산 로직 ===
    XMMATRIX camRotation = XMMatrixRotationRollPitchYaw(mCameraPitch, mCameraYaw, 0.0f);

    XMVECTOR camForward = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), camRotation);
    XMVECTOR camRight = XMVector3TransformNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), camRotation);
    XMVECTOR camUp = XMVector3TransformNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), camRotation);

    // 3. === 키보드 이동(WASD) 로직 ===
    XMVECTOR camPos = XMLoadFloat3(&mCameraPos);
    float moveSpeed = 5.0f * deltaTime;

    if (GetAsyncKeyState('W') & 0x8000) camPos += camForward * moveSpeed;
    if (GetAsyncKeyState('S') & 0x8000) camPos -= camForward * moveSpeed;
    if (GetAsyncKeyState('D') & 0x8000) camPos += camRight * moveSpeed;
    if (GetAsyncKeyState('A') & 0x8000) camPos -= camRight * moveSpeed;
    if (GetAsyncKeyState('E') & 0x8000) camPos += camUp * moveSpeed;
    if (GetAsyncKeyState('Q') & 0x8000) camPos -= camUp * moveSpeed;

    XMStoreFloat3(&mCameraPos, camPos);

    // 4. === 최종 뷰(View) 행렬 업데이트 ===
    XMVECTOR camTarget = camPos + camForward;

    XMMATRIX view = XMMatrixLookAtLH(camPos, camTarget, camUp);
    XMStoreFloat4x4(&mView, view);

    // ====================================================================
    // [3단계: 파이프라인 전송 (W * V * P)]
    // 과거의 1개짜리 큐브 로직을 모두 지우고, 100개짜리로 완전히 통일했습니다!
    // ====================================================================
    static float totalTime = 0.0f;
    totalTime += deltaTime;

    XMMATRIX proj = XMLoadFloat4x4(&mProj);

    // 1. 공통(Pass) 상수 버퍼 업데이트 (카메라의 V * P 1개)
    XMMATRIX viewProj = view * proj;

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
    passConstants.EyePosW = mCameraPos; // 현재 카메라가 위치한 월드 좌표를 셰이더에게 전달합니다. (스페큘러 계산에 필수)



    memcpy(mMappedPassCB, &passConstants, sizeof(PassConstants));

    // 2. 100개 큐브의 고유(Object) 상수 버퍼 배열 업데이트 (각자의 World 행렬들)
    for (int i = 0; i < NumInstances; ++i)
    {
        // 격자 위치 행렬을 불러옵니다.
        XMMATRIX baseWorld = XMLoadFloat4x4(&mWorld[i]);

        // 회원님께서 원하셨던 "큐브 개별 자전 애니메이션"이 바로 이곳에 있습니다! 
        // 큐브마다 제각각 다른 속도와 방향으로 예쁘게 돌도록 i 값을 조금씩 섞어줍니다.
        XMMATRIX rotation = XMMatrixRotationX(totalTime * (1.0f + i * 0.01f)) * XMMatrixRotationY(totalTime * (0.5f + i * 0.02f));
        XMMATRIX finalWorld = rotation * baseWorld; // 자전(회전)한 뒤, 격자 위치로 보냅니다!

        
        // 더 이상 복잡한 주소 계산이 필요 없습니다! 구조체 배열에 바로 대입합니다.
        XMStoreFloat4x4(&mMappedInstanceData[i].World, XMMatrixTranspose(finalWorld));
    }
}

// 매 프레임마다 화면을 렌더링하는 함수의 구현부입니다.
void D3D12Renderer::Draw()
{
    mCommandAllocator->Reset();

    // ========================================================================
    //   [Pass 1: 임시 도화지에 100개의 큐브 숲 그리기]  
    // ========================================================================
    mCommandList->Reset(mCommandAllocator.Get(), mPSO.Get());

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mOffscreenTex.Get(),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
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

    CD3DX12_GPU_DESCRIPTOR_HANDLE passCbvHandle(heapStart, 0, mCbvSrvUavDescriptorSize);
    mCommandList->SetGraphicsRootDescriptorTable(0, passCbvHandle);

    CD3DX12_GPU_DESCRIPTOR_HANDLE texSrvHandle(heapStart, 1, mCbvSrvUavDescriptorSize);
    mCommandList->SetGraphicsRootDescriptorTable(1, texSrvHandle);

    CD3DX12_GPU_DESCRIPTOR_HANDLE instSrvHandle(heapStart, 2, mCbvSrvUavDescriptorSize);
    mCommandList->SetGraphicsRootDescriptorTable(2, instSrvHandle);

    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
    mCommandList->IASetIndexBuffer(&mIndexBufferView);

    mCommandList->DrawIndexedInstanced(mIndexCount, NumInstances, 0, 0, 0);

    // 임시 도화지(mOffscreenTex)에 그림을 다 그렸으므로 다시 READ 상태로 돌려놓습니다!
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mOffscreenTex.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_GENERIC_READ
    );
    mCommandList->ResourceBarrier(1, &barrier);

    // ========================================================================
    //   [Pass 2: 진짜 모니터 화면에 필터(포스트 프로세스) 먹인 그림 그리기]  
    // ========================================================================

    // 1. 드디어 실제 모니터 버퍼(스왑 체인)를 타겟으로 지정합니다!
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mSwapChainBuffer[mCurrBackBuffer].Get(), // 대상: 모니터 버퍼
        D3D12_RESOURCE_STATE_PRESENT,            // 이전: 화면 출력 상태
        D3D12_RESOURCE_STATE_RENDER_TARGET       // 이후: 그림 그리기 상태
    );
    mCommandList->ResourceBarrier(1, &barrier); // 상태 변경 명령

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, mRtvDescriptorSize);

    // 화면 전체를 덮는 필터를 그릴 때는 입체감이 필요 없으므로 깊이 버퍼(dsvHandle)를 nullptr로 꺼버립니다!
    mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // 2. 포스트 프로세싱 전용 계약서와 파이프라인(PSO)으로 교체합니다!
    mCommandList->SetGraphicsRootSignature(mPostRootSignature.Get()); // 새로운 계약서 장착
    mCommandList->SetPipelineState(mPostPSO.Get()); // 새로운 공장 라인 장착

    // 3. 서랍장의 4번째 칸(Index 3)에 있는 '우리가 방금 그린 임시 도화지'를 셰이더에 넘겨줍니다!
    CD3DX12_GPU_DESCRIPTOR_HANDLE postTexSrvHandle(heapStart, 3, mCbvSrvUavDescriptorSize);
    mCommandList->SetGraphicsRootDescriptorTable(0, postTexSrvHandle); // 포스트 루트 시그니처의 0번 슬롯에 장착

    // 4. 마법의 그리기 명령! (버텍스 버퍼 없이 점 3개 던지기)
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 삼각형 형태로
    mCommandList->IASetVertexBuffers(0, 0, nullptr); // 버텍스 버퍼는 텅 빈 상태로!
    mCommandList->IASetIndexBuffer(nullptr);         // 인덱스 버퍼도 텅 빈 상태로!

    // 단 3개의 가상 꼭짓점(SV_VertexID 0, 1, 2)을 쏘아 보내어 화면 전체를 덮는 1개의 거대한 삼각형을 그립니다.
    mCommandList->DrawInstanced(3, 1, 0, 0);

    // 5. 모니터 버퍼에 필터 그림이 다 입혀졌으니 다시 화면 출력(PRESENT) 상태로 복구합니다.
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mSwapChainBuffer[mCurrBackBuffer].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    mCommandList->ResourceBarrier(1, &barrier);
    // ========================================================================

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

// --- 새롭게 업데이트된 BuildGeometry (정육면체 버텍스 + 인덱스 버퍼) ---
bool D3D12Renderer::BuildGeometry()
{
    // 모든 꼭짓점에 텍스처를 씌우기 위한 종이접기 전개도 좌표(UV 좌표, 0.0 ~ 1.0)를 추가합니다.
     // 텍스처의 색상이 그대로 보이도록 꼭짓점 기본 색상은 순백색(1.0, 1.0, 1.0)으로 통일합니다.
    std::vector<Vertex> vertices =
    {
        // 1. 앞면 (Front Face)
        { {-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f} }, // 좌하 (U:0, V:1)
        { {-0.5f, +0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} }, // 좌상 (U:0, V:0)
        { {+0.5f, +0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f} }, // 우상 (U:1, V:0)
        { {+0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} }, // 우하 (U:1, V:1)

        // 2. 뒷면 (Back Face)
        { {-0.5f, -0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} },
        { {+0.5f, -0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} },
        { {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} },
        { {-0.5f, +0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} },

        // 3. 윗면 (Top Face)
        { {-0.5f, +0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} },
        { {-0.5f, +0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} },
        { {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
        { {+0.5f, +0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} },

        // 4. 아랫면 (Bottom Face)
        { {-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} },
        { {+0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} },
        { {+0.5f, -0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} },
        { {-0.5f, -0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} },

        // 5. 왼쪽 면 (Left Face)
        { {-0.5f, -0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} },
        { {-0.5f, +0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },
        { {-0.5f, +0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} },
        { {-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} },

        // 6. 오른쪽 면 (Right Face)
        { {+0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} },
        { {+0.5f, +0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },
        { {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} },
        { {+0.5f, -0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} }
    };

    std::vector<std::uint16_t> indices =
    {
        // 꼭짓점이 24개로 늘어났으므로, 인덱스 번호도 각 면(4개의 점)에 맞게 새로 이어줍니다.
        0, 1, 2,  0, 2, 3,       // 앞면
        4, 5, 6,  4, 6, 7,       // 뒷면
        8, 9, 10, 8, 10, 11,     // 윗면
        12, 13, 14, 12, 14, 15,  // 아랫면
        16, 17, 18, 16, 18, 19,  // 왼쪽 면
        20, 21, 22, 20, 22, 23   // 오른쪽 면
    };

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex); // 정점 버퍼의 총 바이트 크기
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t); // 인덱스 버퍼의 총 바이트 크기
    mIndexCount = (UINT)indices.size(); // 총 그려야 할 인덱스 개수(36개) 저장

    // --- 버텍스 버퍼(Vertex Buffer) GPU에 생성 및 복사 ---
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize);

    mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mVertexBuffer));

    UINT8* pVertexDataBegin;
    mVertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pVertexDataBegin));
    memcpy(pVertexDataBegin, vertices.data(), vbByteSize);
    mVertexBuffer->Unmap(0, nullptr);

    mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
    mVertexBufferView.StrideInBytes = sizeof(Vertex);
    mVertexBufferView.SizeInBytes = vbByteSize;

    // --- 인덱스 버퍼(Index Buffer) GPU에 생성 및 복사 ---
    CD3DX12_RESOURCE_DESC ibDesc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize);

    mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &ibDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mIndexBuffer)); // 인덱스용 GPU 메모리 할당

    UINT8* pIndexDataBegin;
    mIndexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pIndexDataBegin)); // 메모리 열기
    memcpy(pIndexDataBegin, indices.data(), ibByteSize); // C++ 번호표 데이터를 GPU로 복사!
    mIndexBuffer->Unmap(0, nullptr); // 메모리 닫기

    // GPU가 이 버퍼를 '인덱스'로 읽을 수 있도록 뷰(View)를 세팅합니다.
    mIndexBufferView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
    mIndexBufferView.Format = DXGI_FORMAT_R16_UINT; // 16비트 부호없는 정수(uint16_t) 포맷을 사용합니다.
    mIndexBufferView.SizeInBytes = ibByteSize;

    return true;
}
// --------------------------------------------------------------------------

bool D3D12Renderer::BuildConstantBuffers()
{
    //서랍장을 3칸으로 아주 깔끔하게 줄입니다! 
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
     //   [14단계 중요 수정] 서랍장의 크기를 3칸에서 4칸으로 늘립니다!  
    cbvHeapDesc.NumDescriptors = 4; // 0:Pass, 1:Texture, 2:Instance, 3:Offscreen SRV
    //   =========================================================================  
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    mDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap));

    // --- 0번 칸: Pass (공통 데이터) ---
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

    // --- 1번 칸: Texture (체크무늬) ---
    hDescriptor.Offset(1, mCbvSrvUavDescriptorSize); // 한 칸 뒤로 이동

    D3D12_SHADER_RESOURCE_VIEW_DESC texSrvDesc = {};
    texSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    texSrvDesc.Format = mTexture->GetDesc().Format;
    texSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    texSrvDesc.Texture2D.MostDetailedMip = 0;
    texSrvDesc.Texture2D.MipLevels = mTexture->GetDesc().MipLevels;
    texSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    mDevice->CreateShaderResourceView(mTexture.Get(), &texSrvDesc, hDescriptor);

    // --- 2번 칸: Instance Data 배열 (100개 큐브 명부) ---
    hDescriptor.Offset(1, mCbvSrvUavDescriptorSize); // 한 칸 더 뒤로 이동

    // 구조체 배열(SRV)은 256바이트 정렬 규칙을 지키지 않아도 됩니다! 아주 촘촘하게 배열을 만듭니다.
    UINT instanceBufferSize = sizeof(InstanceData) * NumInstances;
    CD3DX12_RESOURCE_DESC instBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(instanceBufferSize);

    mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &instBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mInstanceBuffer));
    mInstanceBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedInstanceData));

    // 이 버퍼가 단순 데이터 배열(Structured Buffer)임을 GPU에 알려주는 뷰(SRV)를 만듭니다.
    D3D12_SHADER_RESOURCE_VIEW_DESC instSrvDesc = {};
    instSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    instSrvDesc.Format = DXGI_FORMAT_UNKNOWN; // 구조체 배열은 UNKNOWN 포맷을 씁니다.
    instSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    instSrvDesc.Buffer.FirstElement = 0;
    instSrvDesc.Buffer.NumElements = NumInstances; // 총 100개
    instSrvDesc.Buffer.StructureByteStride = sizeof(InstanceData); // 구조체 1개당 크기
    instSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    mDevice->CreateShaderResourceView(mInstanceBuffer.Get(), &instSrvDesc, hDescriptor);
   
    //   [14단계 추가] 서랍장의 마지막 4번째 칸에, 방금 그려둔 '임시 도화지'를 텍스처(SRV)로 해석하는 안경을 꽂아둡니다!  
    hDescriptor.Offset(1, mCbvSrvUavDescriptorSize); // 포인터를 4번째 칸으로 이동합니다.

    D3D12_SHADER_RESOURCE_VIEW_DESC offscreenSrvDesc = {}; // SRV 안경의 스펙을 적습니다.
    offscreenSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    offscreenSrvDesc.Format = mOffscreenTex->GetDesc().Format; // 도화지의 원래 포맷(R8G8B8A8)과 동일하게 맞춥니다.
    offscreenSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2D 텍스처로 읽습니다.
    offscreenSrvDesc.Texture2D.MostDetailedMip = 0; // 최고 화질부터
    offscreenSrvDesc.Texture2D.MipLevels = 1; // 1장만 읽습니다.
    offscreenSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    // 서랍장 마지막 칸에 뷰를 만들어 넣습니다! 이제 셰이더에서 이 그림을 텍스처처럼 꺼내 쓸 수 있습니다.
    mDevice->CreateShaderResourceView(mOffscreenTex.Get(), &offscreenSrvDesc, hDescriptor);
    //   =========================================================================  


    return true; // 성공 반환
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

//  [변경점 시작] 화면 밖 임시 렌더 타겟(도화지) 생성 함수 구현! 
bool D3D12Renderer::BuildOffscreenRenderTarget()
{
    // 1. 임시 도화지로 쓸 텍스처(Texture2D) 메모리의 스펙을 정의합니다.
    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC)); // 구조체의 모든 값을 0으로 안전하게 초기화합니다.
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2차원(2D) 텍스처 형태로 설정합니다.
    texDesc.Alignment = 0; // 메모리 정렬을 기본값으로 둡니다.
    texDesc.Width = mClientWidth; // 텍스처의 가로 해상도를 현재 윈도우 창의 너비와 똑같이 맞춥니다.
    texDesc.Height = mClientHeight; // 텍스처의 세로 해상도를 창의 높이와 똑같이 맞춥니다.
    texDesc.DepthOrArraySize = 1; // 텍스처는 1장만 필요합니다.
    texDesc.MipLevels = 1; // 원본 해상도 1개(레벨 1)만 사용합니다.
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 모니터와 완벽히 동일한 RGBA 8비트 색상 포맷을 사용합니다.
    texDesc.SampleDesc.Count = 1; // 안티앨리어싱(MSAA) 샘플 수를 1로 둡니다.
    texDesc.SampleDesc.Quality = 0; // MSAA 품질을 0으로 둡니다.
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // 텍스처 메모리 내부 배치를 그래픽 드라이버가 알아서 최적화하도록 맡깁니다.

    // ★ 가장 중요: 이 텍스처 메모리를 "그림을 그릴 도화지(RENDER_TARGET)" 용도로 쓸 수 있게 허가증을 발급합니다.
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    // 화면 지우기(Clear) 성능을 비약적으로 높이기 위해, 초기에 지울 최적의 색상값(하늘색)을 텍스처 생성 시 미리 알려줍니다.
    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 텍스처 포맷과 동일하게 맞춥니다.
    optClear.Color[0] = 0.39f; // 지울 배경색의 Red 값 (하늘색)입니다.
    optClear.Color[1] = 0.58f; // 지울 배경색의 Green 값입니다.
    optClear.Color[2] = 0.93f; // 지울 배경색의 Blue 값입니다.
    optClear.Color[3] = 1.0f;  // 지울 배경색의 Alpha(불투명도) 값입니다.

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT); // 가장 빠르고 GPU 전용인 VRAM(DEFAULT) 힙을 사용합니다.

    // 2. 정의한 스펙대로 실제 GPU 메모리에 임시 도화지 텍스처를 창조(Create)합니다!
    HRESULT hr = mDevice->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, // ★ 렌더링이 시작되기 전이므로, 초기 상태를 '대기(READ)' 상태로 둡니다.
        &optClear, IID_PPV_ARGS(&mOffscreenTex)); // 생성된 텍스처 객체를 mOffscreenTex 포인터에 담습니다.
    if (FAILED(hr)) return false; // 생성에 실패하면 프로그램을 중단합니다.

    // 3. 이 임시 도화지를 파이프라인(출력 병합기)에 묶을 때 사용할 RTV(안경) 전용 서랍장을 딱 1칸짜리로 만듭니다.
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = 1; // 서랍장 칸은 1개입니다.
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // 서랍장의 종류를 RTV(Render Target View)로 지정합니다.
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // 셰이더에서 이 서랍장을 볼 일은 없으므로 NONE을 줍니다.
    rtvHeapDesc.NodeMask = 0; // 단일 GPU 모드로 설정합니다.
    mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mOffscreenRtvHeap)); // 서랍장 객체를 생성합니다.

    // 4. 마지막으로 생성한 임시 도화지(mOffscreenTex)에 RTV 안경을 예쁘게 씌워서 방금 만든 서랍장(mOffscreenRtvHeap)에 넣습니다.
    mDevice->CreateRenderTargetView(mOffscreenTex.Get(), nullptr, mOffscreenRtvHeap->GetCPUDescriptorHandleForHeapStart());

    // 이 그림을 나중에 포스트 프로세스 셰이더로 넘겨주기 위한 SRV 작업은 다음 단계에서 진행합니다.
    return true; // 무사히 도화지가 완성되었음을 반환합니다.
}
//  ========================================================================= 

//   [14단계 추가] 포스트 프로세싱용 계약서와 조립 라인(PSO)을 만드는 함수입니다.  
bool D3D12Renderer::BuildPostProcessPipeline()
{
    // 1. 포스트 프로세싱용 계약서(루트 시그니처) 작성
    // 임시 도화지(OffscreenTex) 텍스처 하나만 t0 레지스터로 받으면 끝입니다!
    CD3DX12_DESCRIPTOR_RANGE srvTable;
    srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 레지스터 매핑

    CD3DX12_ROOT_PARAMETER rootParameters[1];
    rootParameters[0].InitAsDescriptorTable(1, &srvTable); // 계약 조항은 단 1개

    // 원본 도화지를 부드럽게 가져오기 위해 필터를 LINEAR(뭉개기)로 설정한 샘플러를 만듭니다.
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP; // 화면 밖을 벗어나면 테두리 색으로 늘어뜨림
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    sampler.ShaderRegister = 0; // s0
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
    // 입력 정점 구조체(InputLayout)를 안 쓰기 때문에 ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT 플래그를 뺍니다!
    rootSigDesc.Init(1, rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> serializedRootSig, errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob);
    if (FAILED(hr)) return false;

    // 포스트 전용 루트 시그니처 완성!
    hr = mDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&mPostRootSignature));

    // 2. 포스트 전용 셰이더 컴파일 (새로 만든 postprocess.hlsl 파일을 로드합니다)
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    UINT compileFlags = 0;
#if defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    // postprocess.hlsl 파일에서 VSMain과 PSMain을 컴파일하여 가져옵니다.
    hr = D3DCompileFromFile(L"Source/Shaders/postprocess.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &errorBlob);
    if (FAILED(hr)) { if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer()); return false; }
    hr = D3DCompileFromFile(L"Source/Shaders/postprocess.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &errorBlob);
    if (FAILED(hr)) { if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer()); return false; }

    // 3. 포스트 전용 파이프라인(PSO) 설정
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    // 거대한 삼각형 꼼수를 쓰기 때문에 버텍스 버퍼 구조체(InputLayout)가 아예 필요 없습니다! (텅 빈 배열 전달)
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.pRootSignature = mPostRootSignature.Get();
    psoDesc.VS = { reinterpret_cast<BYTE*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<BYTE*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // 화면 덮는 삼각형이 잘리지 않게 컬링 끔
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // 블렌딩 없음 (원본 위에 덮어씌움)

    // 화면 전체에 무조건 덮어씌울 거니까 앞뒤 거리 재기(Depth Test)도 완전히 꺼버립니다!
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    // 포스트 전용 공장 라인 완성!
    hr = mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPostPSO));
    return SUCCEEDED(hr);
}
//   =========================================================================  
