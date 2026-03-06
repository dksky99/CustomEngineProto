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
    if (!BuildConstantBuffers()) return false; // 4. 상수 버퍼 세팅 

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

        ObjectConstants objConstants;
        XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(finalWorld));

        // GPU 메모리에 100번 연속으로 복사해 줍니다.
        UINT objCBByteSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));
        auto mappedAddress = reinterpret_cast<uint8_t*>(mMappedObjectCB) + (i * objCBByteSize);
        memcpy(mappedAddress, &objConstants, sizeof(ObjectConstants));
    }
}

// 매 프레임마다 화면을 렌더링하는 함수의 구현부입니다.
void D3D12Renderer::Draw()
{ // Draw 함수 시작
    mCommandAllocator->Reset();

    // 파이프라인(mPSO)을 전달하며 명령서를 엽니다.
    mCommandList->Reset(mCommandAllocator.Get(), mPSO.Get());

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mSwapChainBuffer[mCurrBackBuffer].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    mCommandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
        mCurrBackBuffer,
        mRtvDescriptorSize
    );
    // --- 깊이 버퍼 추가 적용 ---
    // 방금 만든 DSV 서랍장의 주소도 가져옵니다.
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(mDsvHeap->GetCPUDescriptorHandleForHeapStart());

    const float clearColor[] = { 0.39f, 0.58f, 0.93f, 1.0f };
    // 화면(RTV)을 지웁니다.
    mCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // 이전 프레임의 흔적을 지우기 위해 깊이 버퍼(DSV) 역시 1.0f(가장 멀리 있는 상태)로 깨끗하게 지워줍니다!
    mCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    // ---------------------------

    // ===  정육면체 그리기 명령들 ===

    mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    mCommandList->RSSetViewports(1, &mViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // 셰이더가 사용할 "상수 버퍼 서랍장(CBV Heap)"을 파이프라인에 가져다 놓습니다.
    ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // 루트 시그니처(계약서) 세팅하기
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    // [변경점 시작] 렌더링 호출 (Draw Call)의 대격변! ====================================
    // 루트 시그니처의 0번(b0) 파라미터에는 공통(Pass) 상수 버퍼를 꽂습니다. (서랍장 0번째 칸)
    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

    // 루트 시그니처의 2번(t0) 파라미터에는 텍스처를 꽂습니다. (서랍장의 마지막 칸)
    CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
    srvHandle.Offset(101, mCbvSrvUavDescriptorSize); // [Pass 1칸] + [Object 100칸] 뒤에 SRV가 있습니다!
    mCommandList->SetGraphicsRootDescriptorTable(2, srvHandle);

    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 삼각형 리스트 지정
    mCommandList->IASetVertexBuffers(0, 1, &mVertexBufferView); // 버텍스 버퍼(큐브 1개 모델링 데이터) 꽂기
    mCommandList->IASetIndexBuffer(&mIndexBufferView); // 인덱스 버퍼 꽂기

    // 인스턴싱 드로우 루프 
    // Draw를 100번 부르는 것은 맞지만, 버퍼와 모델 데이터(큐브)를 다시 세팅할 필요 없이
    // '몇 번째 큐브 데이터(World 행렬)를 읽어올지' 위치만 바꿔가며 100번 초고속으로 발사합니다!
    CD3DX12_GPU_DESCRIPTOR_HANDLE objCbvHandle(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
    objCbvHandle.Offset(1, mCbvSrvUavDescriptorSize); // 첫 번째 Object CBV 칸(인덱스 1)으로 이동

    for (int i = 0; i < NumInstances; ++i)
    {
        // 루트 시그니처의 1번(b1) 파라미터에 "i번째 큐브의 고유 행렬 데이터"를 꽂아줍니다!
        mCommandList->SetGraphicsRootDescriptorTable(1, objCbvHandle);

        // "그려라!" (큐브 1개 분량의 인덱스 36개를 1번 그립니다.)
        mCommandList->DrawIndexedInstanced(mIndexCount, 1, 0, 0, 0);

        // 다음 큐브 행렬 데이터 칸으로 포인터 이동
        objCbvHandle.Offset(1, mCbvSrvUavDescriptorSize);
    }

    // 4. 리소스 배리어 복구 
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mSwapChainBuffer[mCurrBackBuffer].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    mCommandList->ResourceBarrier(1, &barrier);

    mCommandList->Close();
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, cmdsLists);

    mSwapChain->Present(0, 0);
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    FlushCommandQueue();
} // Draw 함수 끝

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

    // 2번 조항: 고유(Object) 상수 버퍼 (b1) - 서랍장 형태
    CD3DX12_DESCRIPTOR_RANGE cbvObjectTable;
    cbvObjectTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // b1

    // 3번 조항: 텍스처 (t0) - 서랍장 형태
    CD3DX12_DESCRIPTOR_RANGE srvTable;
    srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0

    // 파라미터가 총 3개로 늘어납니다!
    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};
    rootParameters[0].InitAsDescriptorTable(1, &cbvPassTable);    // 인덱스 0번: Pass
    rootParameters[1].InitAsDescriptorTable(1, &cbvObjectTable);  // 인덱스 1번: Object
    rootParameters[2].InitAsDescriptorTable(1, &srvTable);        // 인덱스 2번: Texture

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
    // --- 파이프라인 깊이 및 컬링 상태 업데이트 ---
    // 이제 깊이 버퍼가 생겼으므로, 뒷면(Back)을 그리지 않도록 최적화(Culling)를 다시 켭니다!
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

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
    // CBV/SRV 서랍장을 더 크게 만들고, 버퍼도 2개(Pass, Object)를 생성합니다! 
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    // 서랍장 총 개수: [Pass 1개] + [Object 배열 100개] + [Texture 1개] = 총 102칸이 필요합니다!
    cbvHeapDesc.NumDescriptors = 1 + NumInstances + 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    mDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap));

    // --- 1. Pass(공통) 버퍼 생성 및 첫 번째 칸(인덱스 0)에 꽂기 ---
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
    mDevice->CreateConstantBufferView(&passCbvDesc, hDescriptor); // 0번에 장착!


    // --- 2. Object(고유) 버퍼 배열(100개) 생성 및 1~100번 칸에 연달아 꽂기 ---
    UINT objCBByteSize = CalcConstantBufferByteSize(sizeof(ObjectConstants));
    // 100개 분량의 거대한 메모리 덩어리 1개를 할당합니다.
    CD3DX12_RESOURCE_DESC objBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(objCBByteSize * NumInstances);

    mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &objBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mObjectCB));
    mObjectCB->Map(0, nullptr, reinterpret_cast<void**>(&mMappedObjectCB));

    // 100번 반복하면서 서랍장의 1번부터 100번 칸에 차례대로 "배열의 n번째 조각을 읽어라!" 하고 뷰를 꽂아줍니다.
    for (int i = 0; i < NumInstances; ++i)
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC objCbvDesc;
        // 거대한 버퍼의 시작점에서 i번째 크기만큼 점프한 주소를 알려줍니다.
        objCbvDesc.BufferLocation = mObjectCB->GetGPUVirtualAddress() + (i * objCBByteSize);
        objCbvDesc.SizeInBytes = objCBByteSize;

        hDescriptor.Offset(1, mCbvSrvUavDescriptorSize); // 서랍장 한 칸 뒤로 이동
        mDevice->CreateConstantBufferView(&objCbvDesc, hDescriptor); // i번에 장착!
    }

    // --- 3. 마지막 101번 칸에 Texture 뷰(SRV) 꽂기 ---
    hDescriptor.Offset(1, mCbvSrvUavDescriptorSize); // 또 한 칸 뒤로 이동

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = mTexture->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = mTexture->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    mDevice->CreateShaderResourceView(mTexture.Get(), &srvDesc, hDescriptor); // 마지막 칸에 장착!
   

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
            textureData[index + 3] = 255;       // A
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