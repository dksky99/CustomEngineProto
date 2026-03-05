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

    // 오브젝트의 기본 위치(월드 행렬)를 단위 행렬(변형 없음)로 초기화합니다.
    XMStoreFloat4x4(&mWorld, XMMatrixIdentity());
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
    POINT currentMousePos; // 현재 마우스 위치를 담을 변수입니다.
    GetCursorPos(&currentMousePos); // Windows API를 이용해 화면상 마우스 픽셀 위치를 가져옵니다.

    // GetAsyncKeyState 함수를 사용해 마우스 우클릭(VK_RBUTTON)이 현재 눌려있는지 확인합니다. (0x8000 마스크 비트 확인)
    if ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0)
    {
        // 이전 프레임 대비 마우스가 가로(x), 세로(y)로 얼마나 픽셀 이동했는지(Delta) 구합니다.
        // 마우스 감도를 낮추기 위해 0.25f를 곱해주고, 각도(Radian)로 변환(XMConvertToRadians)합니다.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(currentMousePos.x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(currentMousePos.y - mLastMousePos.y));

        mCameraYaw += dx; // 가로 움직임은 좌우 회전(Yaw)에 더해줍니다.
        mCameraPitch += dy; // 세로 움직임은 상하 회전(Pitch)에 더해줍니다.

        // 플레이어가 목을 위아래로 꺾다가 한 바퀴 돌아버리지(뒤집히지) 않도록, 상하 각도를 -89도 ~ +89도 정도로 제한(Clamp)합니다.
        mCameraPitch = std::clamp(mCameraPitch, -1.5f, 1.5f); // 1.5 라디안은 약 86도입니다.
    }
    mLastMousePos = currentMousePos; // 다음 프레임 계산을 위해 현재 위치를 과거 위치로 업데이트합니다.

    // 2. === 방향 벡터 계산 로직 ===
    // 방금 구한 상하/좌우 회전각(Pitch, Yaw)을 바탕으로 '카메라의 회전 행렬'을 만듭니다.
    XMMATRIX camRotation = XMMatrixRotationRollPitchYaw(mCameraPitch, mCameraYaw, 0.0f);

    // 기본 방향(앞: Z축, 오른쪽: X축, 위쪽: Y축) 벡터들에 방금 만든 회전 행렬을 곱해서 '카메라 기준의 진짜 방향 벡터'를 얻어냅니다.
    XMVECTOR camForward = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), camRotation); // 카메라가 바라보는 앞방향
    XMVECTOR camRight = XMVector3TransformNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), camRotation); // 카메라 기준 오른쪽
    XMVECTOR camUp = XMVector3TransformNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), camRotation); // 카메라 기준 위쪽

    // 3. === 키보드 이동(WASD) 로직 ===
    XMVECTOR camPos = XMLoadFloat3(&mCameraPos); // 멤버 변수에 저장된 카메라 위치를 SIMD 연산용 벡터로 불러옵니다.
    float moveSpeed = 5.0f * deltaTime; // 1초에 5 유닛(미터)씩 이동하도록 속도를 설정합니다.

    // GetAsyncKeyState로 W, A, S, D, Q, E 키가 눌려있는지 실시간으로 확인합니다.
    if (GetAsyncKeyState('W') & 0x8000) camPos += camForward * moveSpeed; // 앞(Forward)으로 이동
    if (GetAsyncKeyState('S') & 0x8000) camPos -= camForward * moveSpeed; // 뒤로 이동
    if (GetAsyncKeyState('D') & 0x8000) camPos += camRight * moveSpeed;   // 우측(Right)으로 이동 (게걸음)
    if (GetAsyncKeyState('A') & 0x8000) camPos -= camRight * moveSpeed;   // 좌측으로 이동
    if (GetAsyncKeyState('E') & 0x8000) camPos += camUp * moveSpeed;      // 위(Up)로 상승 (수직 비행)
    if (GetAsyncKeyState('Q') & 0x8000) camPos -= camUp * moveSpeed;      // 아래로 하강

    XMStoreFloat3(&mCameraPos, camPos); // 키보드 조작으로 갱신된 위치를 다시 멤버 변수에 저장해 둡니다.

    // 4. === 최종 뷰(View) 행렬 업데이트 ===
    // 카메라 위치(Pos)에 카메라 앞방향(Forward)을 더해서 '카메라가 바라보는 목표 지점(Target)'을 계산합니다.
    XMVECTOR camTarget = camPos + camForward;

    // (위치, 목표 지점, 위쪽 방향) 이 3가지 정보를 조합하여 최종 뷰 행렬을 만들어냅니다!
    XMMATRIX view = XMMatrixLookAtLH(camPos, camTarget, camUp);
    XMStoreFloat4x4(&mView, view); // 멤버 변수 갱신


    static float totalTime = 0.0f; // 누적 시간을 저장할 정적 변수입니다.
    totalTime += deltaTime; // 프레임 간의 시간(DeltaTime)을 계속 더합니다.

    // 큐브가 더 멋지게 보이도록 X축과 Y축 양방향으로 회전시킵니다!
    XMMATRIX world = XMMatrixRotationX(totalTime * 0.5f) * XMMatrixRotationY(totalTime);
    XMStoreFloat4x4(&mWorld, world); // 갱신된 월드 행렬을 저장합니다.

    // 월드, 투영 행렬을 메모리에서 불러옵니다.
    XMMATRIX proj = XMLoadFloat4x4(&mProj);

    // 3개의 행렬을 곱하여 최종 변환 행렬(World * View * Projection)을 만듭니다.
    XMMATRIX worldViewProj = world * view * proj;

    // GPU로 보낼 구조체에 데이터를 채웁니다. 
    ObjectConstants objConstants;
    // HLSL 셰이더는 기본적으로 열 우선(Column-Major) 행렬을 쓰므로, C++의 행 우선(Row-Major) 행렬을 전치(Transpose)해서 넘겨주어야 합니다.
    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));

    // 2. 새롭게 추가된 World 행렬을 담습니다. (셰이더에서 법선 벡터를 3D 공간으로 변환할 때 사용됨)
    XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
    // 매핑된 GPU 메모리에 계산된 구조체를 통째로 덮어씌웁니다. (초고속 택배 발송!)
    memcpy(mMappedObjectCB, &objConstants, sizeof(ObjectConstants));
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

    // 서랍장의 첫 번째 칸(방금 Map 해둔 행렬 데이터)을 루트 시그니처의 0번 파라미터(b0)에 연결합니다!
    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

    // 서랍장(mCbvHeap)의 2번째 칸(인덱스 1번)에 넣어둔 '텍스처 뷰(SRV)'를 파이프라인의 1번 파라미터(t0)에 묶어줍니다!
    CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
    srvHandle.Offset(1, mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    mCommandList->SetGraphicsRootDescriptorTable(1, srvHandle);


    // 점들을 이어서 삼각형(TRIANGLELIST)으로 만들라고 지시하기
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 파이프라인에 우리가 만든 정점 버퍼(꼭짓점 8개)를 꽂아 넣습니다.
    mCommandList->IASetVertexBuffers(0, 1, &mVertexBufferView);

    // --- 새롭게 추가된 부분: 파이프라인에 인덱스 버퍼(번호표)를 꽂아 넣습니다! ---
    mCommandList->IASetIndexBuffer(&mIndexBufferView);

    // --- 수정된 부분: DrawInstanced 대신 DrawIndexedInstanced를 사용하여 인덱스 기반으로 그립니다! ---
    // 첫 번째 파라미터(mIndexCount)는 총 36개의 인덱스를 그리라는 의미입니다.
    mCommandList->DrawIndexedInstanced(mIndexCount, 1, 0, 0, 0);
    // ==========================================

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
    // 셰이더에게 "b0 슬롯(상수 버퍼) 1개를 사용할 거야"라고 알려주는 테이블을 만듭니다.
    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // 타입(CBV), 개수(1), 셰이더 레지스터 번호(b0)
    // 루트 시그니처에 "나는 텍스처(SRV) 1개도 받을 것이다(t0)"라는 조항을 추가합니다.
    CD3DX12_DESCRIPTOR_RANGE srvTable;
    srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 레지스터

    // 기존 1개였던 파라미터 배열을 2개로 늘리고, srvTable을 등록합니다.
    CD3DX12_ROOT_PARAMETER rootParameters[2];
    rootParameters[0].InitAsDescriptorTable(1, &cbvTable);
    rootParameters[1].InitAsDescriptorTable(1, &srvTable);

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
    // 파라미터 2개, 샘플러 1개를 등록하여 최종 루트 시그니처를 초기화합니다!
    rootSigDesc.Init(2, rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = 2; // 행렬 서랍장(CBV) 1칸 + 텍스처 서랍장(SRV) 1칸 = 총 2칸으로 늘립니다!
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // ★중요★ 셰이더가 볼 수 있게 허용
    cbvHeapDesc.NodeMask = 0;
    mDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap));
    // --- 1. 첫 번째 칸: 행렬 상수 버퍼(CBV) 꽂기 ---
    UINT objCBByteSize = (sizeof(ObjectConstants) + 255) & ~255; // 256의 배수로 올림 (매크로 대신 직접 식 사용)

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(objCBByteSize);

    mDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mObjectCB));

    mObjectCB->Map(0, nullptr, reinterpret_cast<void**>(&mMappedObjectCB));

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = mObjectCB->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = objCBByteSize;

    mDevice->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());

    // --- 2. 두 번째 칸: 텍스처 뷰(SRV) 꽂기 ---
    // 서랍장의 주소를 가져와서 1칸(SRV 크기만큼) 뒤로 이동시킵니다.
    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
    hDescriptor.Offset(1, mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    // 텍스처를 셰이더 리소스로 쓸 수 있도록 뷰(안경)를 만듭니다.
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = mTexture->GetDesc().Format; // 텍스처 만들 때 썼던 포맷(RGBA)
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = mTexture->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    // 두 번째 칸에 텍스처 뷰(SRV)를 꽂아 넣습니다!
    mDevice->CreateShaderResourceView(mTexture.Get(), &srvDesc, hDescriptor);


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