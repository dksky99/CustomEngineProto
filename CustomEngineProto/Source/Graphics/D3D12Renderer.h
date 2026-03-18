#pragma once// 이 헤더 파일이 여러 번 중복으로 포함되는 것을 방지합니다.

#include <d3d12.h> // DirectX 12의 핵심 기능들을 사용하기 위한 헤더를 포함합니다.
#include <dxgi1_6.h> // 디스플레이 및 스왑 체인을 관리하는 헤더를 포함합니다.
#include <wrl.h> // 스마트 포인터인 ComPtr을 사용하기 위한 헤더를 포함합니다.
#include <DirectXMath.h> // 3D 수학 연산(벡터, 위치 등)을 위한 헤더를 포함합니다.
#include <DirectXPackedVector.h> // 압축된 수학 데이터 타입을 위한 헤더를 포함합니다.
#include "d3dx12.h" // D3D12 구조체 초기화를 돕는 헬퍼 헤더를 포함합니다.

// 렌더러가 새롭게 도입된 씬(Scene) 개념을 인지할 수 있도록 전방 선언(Forward Declaration)을 해줍니다. 
class Scene; // Scene이라는 클래스가 외부에 존재함을 컴파일러에게 미리 알려줍니다.

//  렌더러가 외부에서 전달받을 '카메라(Camera)' 클래스의 존재를 미리 알려줍니다. 
class Camera;


using Microsoft::WRL::ComPtr; // 코드의 가독성을 위해 Microsoft::WRL 네임스페이스의 ComPtr을 사용한다고 선언합니다.
using namespace DirectX; // DirectXMath의 함수들을 편하게 쓰기 위해 네임스페이스를 개방합니다.

// C++에서 삼각형의 꼭짓점 정보를 담을 구조체입니다. (HLSL 셰이더의 VSInput 구조체와 일치해야 함)
struct Vertex
{ // Vertex 구조체 시작
    DirectX::XMFLOAT3 Pos; // 3차원 위치 데이터 (X, Y, Z) 입니다.
    DirectX::XMFLOAT4 Color; // 4차원 색상 데이터 (R, G, B, A) 입니다.
    DirectX::XMFLOAT3 Normal; // --- 새롭게 추가됨: 이 점이 바라보는 방향(법선 벡터)입니다. ---
    // 정점이 텍스처 이미지의 어느 좌표(U, V)를 참조할지 나타내는 2D 데이터입니다.
    DirectX::XMFLOAT2 TexC;
}; // Vertex 구조체 끝

//  [ ] 셰이더와 통신할 공통(Pass) 상수 버퍼 구조체를 업데이트합니다! 
struct PassConstants
{
    DirectX::XMFLOAT4X4 ViewProj = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f }; // V * P 행렬
    DirectX::XMFLOAT3 LightDir = { 0.577f, -0.577f, 0.577f }; // 공통 빛 방향
    float TotalTime = 0.0f; // 추가 1: 텍스처 애니메이션을 위해 셰이더로 넘겨줄 '현재 누적 시간'
    DirectX::XMFLOAT4 LightColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // 공통 빛 색상
    DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f }; // 추가 2: 스페큘러(반사) 계산을 위해 셰이더가 알아야 할 '현재 카메라의 3D 위치'
    float padding = 0.0f; // 16바이트 정렬(Float4) 규칙을 맞추기 위해 빈칸(패딩) 1개를 추가합니다.
};
// CPU에서 계산한 행렬 데이터를 GPU(셰이더)로 넘겨주기 위한 상수 구조체입니다.
// 2. 각각의 큐브(인스턴스)마다 다르게 가질 고유 데이터 (월드 위치/회전)

struct InstanceData
{
    DirectX::XMFLOAT4X4 World = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
};
// 


// 이번 예제에서 그릴 큐브의 총 개수를 상수로 정의합니다! (10 x 10 = 100개)
const int NumInstances = 100;





// 상수 버퍼 크기를 무조건 256바이트의 배수로 맞춰주는 유틸리티 함수입니다. (DX12 하드웨어 필수 규약)
inline UINT CalcConstantBufferByteSize(UINT byteSize)
{ // 유틸리티 함수 시작
    // 하드웨어 제약상 상수 버퍼는 256바이트 단위로 끊어져야 하므로, 공식을 통해 256의 배수로 올림 연산합니다.
    return (byteSize + 255) & ~255;
} // 유틸리티 함수 끝


class D3D12Renderer // DirectX 12의 초기화 및 렌더링을 담당할 메인 클래스입니다.
{ // D3D12Renderer 클래스 시작
public: // 외부에서 호출할 수 있는 퍼블릭 함수들입니다.
    D3D12Renderer(); // 생성자입니다.
    ~D3D12Renderer(); // 소멸자입니다.

    // 윈도우 핸들(HWND)과 창의 너비/높이를 받아 DX12 객체들을 생성하고 초기화하는 함수입니다.
    bool Initialize(HWND hwnd, int width, int height);


    // --- 새롭게 추가된 함수: 매 프레임 수학 연산(행렬 갱신)을 처리할 함수입니다. ---
    void Update(float deltaTime, Scene* scene, Camera* camera);


    void Draw(); // 매 프레임마다 호출되어 화면을 특정 색상으로 지우고(Clear) 화면에 출력(Present)하는 함수입니다.
    void FlushCommandQueue(); // CPU가 GPU의 작업이 모두 끝날 때까지 기다려주는(동기화) 함수입니다.
private:

    //루트 시그니쳐
    bool BuildRootSignature(); // 셰이더와 데이터를 주고받을 규약인 '루트 시그니처'를 생성하는 내부 함수입니다.

    bool BuildPSO(); // 셰이더와 렌더링 상태를 하나로 묶은 파이프라인 객체(PSO)를 만듭니다.

    bool BuildGeometry(); // 삼각형의 정점 데이터를 만들고 GPU 메모리(버퍼)로 보냅니다.


    // 상수 버퍼를 생성하는 함수입니다. ---
    bool BuildConstantBuffers();
    // C++에서 직접 체크무늬 이미지를 만들어 GPU로 올리는 함수입니다.
    bool BuildTexture();

    // [변경점 시작] 임시 도화지(Off-screen Render Target)를 만드는 함수를 추가합니다. 
    bool BuildOffscreenRenderTargets(); // 모니터 대신 그림을 그릴 임시 메모리 텍스처를 생성하는 함수입니다.

    //   [14단계 추가] 포스트 프로세싱용 두 번째 파이프라인(PSO)을 조립하는 함수입니다.  
    bool BuildPostProcessPipelines(); // 필터를 먹이는 공장 라인을 만듭니다.
    //   =========================================================================  
     //   [구조 분화] 복잡했던 Draw 함수를 역할에 맞게 엔진처럼 나눕니다.  
    void RenderScene(); // 3D 모델(큐브)들을 그리는 단계
    void RenderPostProcess(); // 2D 필터(포스트 프로세스)를 체인으로 연결하여 그리는 단계
    //   =========================================================================  
private: // 클래스 내부에서만 접근 가능한 그래픽스 인터페이스 객체들입니다.
    ComPtr<IDXGIFactory4> mDxgiFactory; // 스왑 체인을 생성하고 하드웨어 어댑터를 열거할 DXGI 팩토리 객체입니다.
    ComPtr<ID3D12Device> mDevice; // GPU를 대표하며 리소스를 생성하는 핵심 D3D12 디바이스 객체입니다.

    // DX12에서는 스왑 체인을 만들 때 명령을 전달할 '큐(Queue)'가 반드시 필요하므로 함께 선언합니다.
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<IDXGISwapChain3> mSwapChain; // 더블 버퍼링을 관리하여 화면을 모니터에 출력해 줄 스왑 체인 객체입니다.


    int mClientWidth; // 렌더링할 화면의 너비를 저장하는 변수입니다.
    int mClientHeight; // 렌더링할 화면의 높이를 저장하는 변수입니다.
    HWND mMainWindow; // 화면을 출력할 대상 윈도우의 핸들을 저장하는 변수입니다.

    //커맨드
    ComPtr<ID3D12CommandAllocator> mCommandAllocator; // 커맨드 리스트가 명령을 기록할 때 사용할 메모리를 할당해 주는 객체입니다.
    ComPtr<ID3D12GraphicsCommandList> mCommandList; // GPU에게 내릴 실제 렌더링 명령들을 순서대로 차곡차곡 기록하는 명령서 객체입니다.

    //동기화 및 랜더타겟 뷰
    static const int SwapChainBufferCount = 2; // 더블 버퍼링을 위해 스왑 체인 버퍼 개수를 2개로 고정하는 상수입니다.
    ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount]; // 프론트 버퍼와 백 버퍼의 실제 텍스처 메모리를 가리킬 포인터 배열입니다.
    ComPtr<ID3D12DescriptorHeap> mRtvHeap; // 렌더 타겟 뷰(RTV)들을 저장할 서랍장(디스크립터 힙) 객체입니다.
    UINT mRtvDescriptorSize; // 현재 GPU에서 RTV 디스크립터 한 개가 차지하는 메모리 크기(바이트)를 구하여 저장해둡니다.
    int mCurrBackBuffer = 0; // 현재 화면 뒤에서 몰래 그림을 그리고 있는 백 버퍼의 인덱스(0 또는 1)를 추적합니다.

    //   [구조 분화] 가우시안 핑퐁을 위해 도화지를 3개로 확장합니다.  
    // 0번: 원본 씬, 1번: 밝기 추출 및 최종 세로 블러 (Ping), 2번: 가로 블러 (Pong)
    static const int OffscreenBufferCount = 3;
    ComPtr<ID3D12Resource> mOffscreenTex[OffscreenBufferCount]; // 임시 도화지 3장입니다.
    ComPtr<ID3D12DescriptorHeap> mOffscreenRtvHeap; // 임시 도화지 전용 안경 서랍장입니다.

    //   [구조 분화] 입력 텍스처 개수에 따라 상용 엔진처럼 루트 시그니처(계약서)를 두 가지로 분리합니다.  
    ComPtr<ID3D12RootSignature> mPostRootSigSingle; // 텍스처 1개만 입력받는 계약서 (추출, 가로, 세로 블러용)
    ComPtr<ID3D12RootSignature> mPostRootSigDouble; // 텍스처 2개를 동시에 입력받는 계약서 (최종 합성용)

    // 각 역할별 독립된 파이프라인(공장 라인)을 선언합니다.
    ComPtr<ID3D12PipelineState> mPsoBrightPass; // 빛 추출 라인
    ComPtr<ID3D12PipelineState> mPsoBlurH; // 가로 가우시안 블러 라인
    ComPtr<ID3D12PipelineState> mPsoBlurV; // 세로 가우시안 블러 라인
    ComPtr<ID3D12PipelineState> mPsoComposite; // 두 화면 합성 라인
    //   =========================================================================  

    ComPtr<ID3D12Fence> mFence; // CPU와 GPU의 작업 완료 타이밍을 맞추기 위한 동기화 객체(울타리)입니다.
    UINT64 mCurrentFence = 0; // 현재 CPU가 GPU에게 발급한 대기표(펜스 값) 번호를 저장합니다.

    //루트 시그니쳐
    ComPtr<ID3D12RootSignature> mRootSignature; // 파이프라인에 바인딩할 데이터의 형식을 정의하는 루트 시그니처 객체입니다.


    ComPtr<ID3D12PipelineState> mPSO; // 파이프라인 상태 객체(PSO)입니다.

    //  렌더링 자원들 ---
    ComPtr<ID3D12Resource> mVertexBuffer; // 삼각형의 꼭짓점 데이터를 담고 있을 GPU 메모리 버퍼입니다.
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView; // GPU가 위 버퍼를 어떻게 읽어야 하는지 알려주는 뷰 구조체입니다.


    // ---  인덱스 버퍼 관련 변수들 ---
    ComPtr<ID3D12Resource> mIndexBuffer; // 정점들을 어떤 순서로 이어서 그릴지(번호표)를 담을 GPU 버퍼입니다.
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView; // GPU가 인덱스 버퍼를 어떻게 읽어야 하는지 알려주는 뷰 구조체입니다.
    UINT mIndexCount = 0; // 이번 프레임에 총 몇 개의 인덱스(번호)를 읽어서 그려야 하는지 저장합니다.

    // --- 깊이 버퍼 관련 변수들 ---
    ComPtr<ID3D12Resource> mDepthStencilBuffer; // 화면 픽셀들의 거리(Z값) 정보를 저장할 2D 텍스처 메모리입니다.
    ComPtr<ID3D12DescriptorHeap> mDsvHeap; // 깊이-스텐실 뷰(DSV)를 담을 전용 서랍장입니다.
    // --------------------------------


    // ---  상수 버퍼(CBV) 관련 변수들입니다. ---
     //  상수 버퍼를 2개로 분리합니다! (전역 데이터 1개 + 100개의 큐브 고유 데이터 배열) 
    ComPtr<ID3D12Resource> mPassCB; // 카메라/빛 등 공통(Pass) 데이터를 담을 상수 버퍼입니다.
    PassConstants* mMappedPassCB = nullptr; // Pass 상수 버퍼 CPU 맵핑 포인터

    // 상수 버퍼 배열(CBV)이 아니라, 구조체 버퍼(SRV)를 사용할 것입니다. 
    ComPtr<ID3D12Resource> mInstanceBuffer;
    InstanceData* mMappedInstanceData = nullptr;

    ComPtr<ID3D12DescriptorHeap> mCbvHeap; // 상수 버퍼/텍스처를 담을 서랍장입니다.
    UINT mCbvSrvUavDescriptorSize = 0;// CBV 서랍장 한 칸의 크기를 기억할 변수입니다.




    // 텍스처 이미지 메모리와, 이미지를 GPU로 넘기기 전 사용하는 업로드 메모리입니다.
    ComPtr<ID3D12Resource> mTexture;
    ComPtr<ID3D12Resource> mTextureUploadBuffer;





    D3D12_VIEWPORT mViewport; // 도화지에서 실제로 그림이 그려질 영역(뷰포트) 정보입니다.
    D3D12_RECT mScissorRect; // 렌더링을 잘라낼(Scissor) 사각형 영역 정보입니다.


}; // D3D12Renderer 클래스 끝
