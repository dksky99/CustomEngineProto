#include <windows.h> // Windows API의 기본 기능들을 사용하기 위한 헤더를 포함합니다.
#include <d3d12.h> // DirectX 12의 핵심 기능(디바이스, 커맨드 리스트 등)을 위한 헤더를 포함합니다.
#include <dxgi1_6.h> // DXGI(DirectX Graphics Infrastructure) 버전 1.6 헤더를 포함합니다. (스왑 체인 등 관리)
#include <d3dcompiler.h> // HLSL 셰이더 코드를 컴파일하기 위한 헤더를 포함합니다.
#include <DirectXMath.h> // 벡터, 행렬 등 3D 수학 연산을 돕는 헤더를 포함합니다.
#include "d3dx12.h" // 앞서 NuGet으로 설치한 D3D12 헬퍼 구조체/함수 헤더를 포함합니다.

#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup")
// 프로젝트 설정 대신 코드 내에서 링커에게 필요한 정적 라이브러리(.lib)를 연결하도록 지시합니다.
#pragma comment(lib, "d3d12.lib") // D3D12 핵심 라이브러리를 링크합니다.
#pragma comment(lib, "dxgi.lib") // 그래픽스 인프라스트럭처 라이브러리를 링크합니다.
#pragma comment(lib, "d3dcompiler.lib") // 셰이더 컴파일러 라이브러리를 링크합니다.

#include "Core/EngineConfig.h"
#include "Core/Timer/GameTimer.h"
#include "Graphics/Core/D3D12Renderer.h"

//  [추가점] 구조 개편에 따라 메인 함수에서도 프레임워크 요소(Scene, Actor)들을 사용할 수 있도록 헤더를 포함합니다. 
#include "Framework/Core/Scene.h" // 액터들을 담을 거대한 세상(맵) 객체입니다.
#include "Framework/Core/Actor.h" // 씬에 스폰될 개별 물체 객체입니다.



#include "Game/CubeActor.h"

//  액터에게 외형 부품을 달아주기 위해 메시 컴포넌트 헤더를 포함합니다. 
#include "Framework/Components/MeshComponent.h" 

#include "Framework/Components/CameraComponent.h" 

//   [추가점] 흩어져 있던 마우스/키보드 입력을 처리할 중앙 통제실 헤더를 포함합니다!  
#include "Framework/Core/InputManager.h" 

// 윈도우에서 발생하는 이벤트(키보드 입력, 마우스 클릭, 닫기 버튼 등)를 처리할 콜백 함수의 원형을 선언합니다.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Windows 데스크톱 애플리케이션의 공식적인 진입점(Entry Point) 함수입니다.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{ // WinMain 함수의 시작
    const wchar_t CLASS_NAME[] = L"DX12EngineClass"; // 등록할 윈도우 클래스의 식별자 이름을 와이드 문자열로 정의합니다.

    WNDCLASS wc = { }; // 윈도우의 속성을 정의할 구조체를 선언하고 0으로 초기화합니다.
    wc.lpfnWndProc = WindowProc; // 윈도우 메시지를 처리할 함수 포인터를 연결합니다.
    wc.hInstance = hInstance; // 이 애플리케이션의 현재 실행 인스턴스 핸들을 지정합니다.
    wc.lpszClassName = CLASS_NAME; // 위에서 정의한 윈도우 클래스 이름을 지정합니다.

    RegisterClass(&wc); // 운영체제에 위에서 설정한 윈도우 클래스를 등록합니다.

    EngineConfig config; // 기본값이 채워진 설정 구조체를 하나 생성합니다.

    // 실제 화면에 보여질 윈도우 객체를 메모리에 생성하고 핸들(HWND)을 반환받습니다.
    HWND hwnd = CreateWindowEx(
        0, // 확장 윈도우 스타일은 사용하지 않으므로 0을 줍니다.
        CLASS_NAME, // 방금 등록한 윈도우 클래스 이름을 사용하여 윈도우를 만듭니다.
        config.WindowTitle, // 윈도우 창 상단(타이틀 바)에 표시될 텍스트입니다.
        WS_OVERLAPPEDWINDOW, // 기본적인 창 스타일(제목 표시줄, 크기 조절 테두리, 최소/최대화 버튼 등)을 지정합니다.
        CW_USEDEFAULT, CW_USEDEFAULT, // 창이 뜰 X, Y 좌표를 운영체제의 기본 위치로 맡깁니다.
        config.WindowWidth, config.WindowHeight, // 창의 초기 너비를 1280, 높이를 720 픽셀로 설정합니다.
        NULL, // 부모 윈도우가 없으므로 NULL을 지정합니다. (이것이 메인 윈도우입니다.)
        NULL, // 윈도우 메뉴바를 사용하지 않으므로 NULL을 지정합니다.
        hInstance, // 현재 애플리케이션 인스턴스를 지정합니다.
        NULL // 추가적인 생성 데이터를 넘기지 않으므로 NULL을 지정합니다.
    );

    if (hwnd == NULL) // 윈도우가 정상적으로 생성되지 않았다면 핸들이 NULL이 됩니다.
    { // 실패 처리 블록 시작
        return 0; // 에러가 발생했으므로 프로그램을 종료합니다.
    } // 실패 처리 블록 끝

    ShowWindow(hwnd, nCmdShow); // 메모리에 생성된 윈도우를 실제 모니터 화면에 표시합니다.
   
    D3D12Renderer renderer;
    if (!renderer.Initialize(hwnd, config.WindowWidth, config.WindowHeight))
    {
        MessageBox(0, L"DirectX 12 Initialization Failed.", L"Error", MB_OK);
        return 0;
    }


    //  렌더러가 그림을 그리기 전에, 개발자(우리)가 씬을 1개 만들고 100개의 큐브 액터를 직접 배치하는 레벨 디자인을 수행합니다! 
    Scene scene; // 월드(맵)를 담당할 씬 객체를 메모리에 올립니다.

    //  상용 엔진처럼 액터 생성 -> 컴포넌트 생성 -> 메시 씌우기 -> 액터 조립의 과정을 거칩니다! 
    for (int i = 0; i < 10; ++i) // 루프 시작
    {
        for (int j = 0; j < 10; ++j) // 루프 시작
        {
            int index = i * 10 + j; // 인덱스 계산
            float xPos = (j - 4.5f) * 2.0f; // X 위치 계산
            float zPos = (i - 4.5f) * 2.0f; // Z 위치 계산

            std::shared_ptr<CubeActor> cube = std::make_shared<CubeActor>(); // 빈 액터를 만듭니다.
            cube->CubeIndex = index; // 인덱스 부여
            cube->GetTransform()->Position = { xPos, 0.0f, zPos }; // 초기 위치 부여

            // 1. 빈 메시 컴포넌트(부품)를 하나 메모리에 찍어냅니다.
            std::shared_ptr<MeshComponent> meshComp = std::make_shared<MeshComponent>();
            // 2. 렌더러가 만들어둔 기본 정육면체 모델 데이터의 포인터를 얻어와 부품에 쏙 끼웁니다.
            meshComp->SetMesh(renderer.GetDefaultMesh());
            // 3. 모델 데이터가 들어간 이 껍데기 부품을 액터의 몸에 장착합니다!
            cube->AddComponent(meshComp);

            scene.AddActor(cube); // 모든 조립이 끝난 액터를 씬 명부에 올려줍니다.
        } // 루프 끝
    } // 루프 끝

    
    
    
    GameTimer timer; // 위에서 우리가 만든 게임 타이머 객체의 인스턴스를 생성합니다.
    timer.Reset(); // 본격적인 렌더링 루프에 들어가기 직전, 타이머의 기준점을 현재 시간으로 초기화합니다.

   
    // --- [ECS 아키텍처 완성] 카메라를 '액터(Actor)'로 취급하여 씬에 조립해 넣습니다!  ---

    // 1. 눈에 보이지 않는 텅 빈 투명 액터를 하나 메모리에 찍어냅니다. (이것이 상용 엔진의 CameraActor 입니다!)
    std::shared_ptr<Actor> cameraActor = std::make_shared<Actor>();
    cameraActor->GetTransform()->Position = { 0.0f, 15.0f, -15.0f }; // 공중에 살짝 띄웁니다.
    cameraActor->GetTransform()->Rotation = { 0.7f, 0.0f, 0.0f }; // 밑의 큐브들을 내려다보게 살짝 고개를 숙여줍니다.

    // 2. 화면을 찍어낼 렌즈 부품(CameraComponent)을 생성합니다.
    std::shared_ptr<CameraComponent> cameraComp = std::make_shared<CameraComponent>();
    cameraComp->SetLens(0.25f * XM_PI, static_cast<float>(config.WindowWidth) / config.WindowHeight, 1.0f, 1000.0f);

    // 3. 조립 완료! 액터의 몸통에 카메라 부품을 부착합니다. (이 과정 덕분에 mOwner가 연결되어 에러가 안 납니다!)
    cameraActor->AddComponent(cameraComp);

    // 4. 조립이 끝난 카메라 액터를 게임 씬(명부)에 등록하여 관리망에 넣습니다.
    scene.AddActor(cameraActor);
    // -----------------------------------------------------------------------------------

    MSG msg = { 0 }; // 운영체제로부터 받을 메시지를 담을 구조체를 0으로 비워 선언합니다.

    // msg.message가 프로그램 종료를 뜻하는 WM_QUIT이 아닐 때까지 무한히 반복하는 메인 게임 루프입니다.
    while (msg.message != WM_QUIT)
    { // 게임 루프 시작
        // PeekMessage는 메시지 큐를 확인하여 메시지가 있으면 true, 없으면 false를 즉시 반환하며 기다리지 않습니다.
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        { // 메시지가 존재할 경우 처리하는 블록 시작
            TranslateMessage(&msg); // 키보드 눌림 등의 가상 키 메시지를 문자 메시지로 번역합니다.
            DispatchMessage(&msg); // 번역된 메시지를 우리가 만든 WindowProc 윈도우 프로시저로 보냅니다.
        } // 메시지 처리 블록 끝
        else
        { // 처리할 윈도우 메시지가 없다면 (이곳이 바로 매 프레임 게임 로직이 실행되는 핵심 구역입니다)
            timer.Tick(); // 매 프레임 타이머를 한 번 틱(Tick)하여 최신 델타 타임(DeltaTime)을 계산해 냅니다.
            // TODO: Update(timer.DeltaTime()); // 계산된 델타 타임을 전달하여 게임 속 오브젝트들의 상태(위치, 물리 등)를 갱신합니다.
            
            //   1단계: 이번 프레임의 마우스/키보드 입력을 전역 통제 센터에 수집합니다!  
            InputManager::GetInstance().Update();

            //   2단계: 씬 갱신! (이때 카메라 액터 안에 달린 CameraComponent도 씬을 통해 자동으로 Update 됩니다!)  
            scene.Update(timer.DeltaTime());

            //   3단계: 렌더러에게 액터 명부와 카메라 부품(시점)을 던져주고 화면을 그리게 시킵니다.  
            // 카메라 포인터(get)를 렌더러에 넘겨줍니다.
            renderer.Update(timer.DeltaTime(), &scene, cameraComp.get());

            renderer.Draw(); // 2. 렌더러를 통해 화면 렌더링 호출!
        } // 게임 로직 블록 끝
    } // 게임 루프 끝

    return (int)msg.wParam; // 루프를 무사히 빠져나오면, WM_QUIT 메시지에 담겨있던 종료 코드를 반환하며 프로그램을 마칩니다.
} // WinMain 함수의 끝

// 메시지를 처리하는 윈도우 프로시저 함수의 구현부입니다.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ // WindowProc 함수의 시작
    switch (uMsg) // 발생한 메시지의 종류(uMsg)에 따라 어떤 동작을 할지 분기합니다.
    { // switch 블록 시작
    case WM_DESTROY: // 사용자가 윈도우의 'X' 버튼을 눌러 창이 파괴될 때 발생하는 메시지입니다.
        PostQuitMessage(0); // 메시지 큐에 WM_QUIT 메시지를 넣어 WinMain의 GetMessage 루프를 탈출하게 합니다.
        return 0; // 이 메시지를 우리가 직접 처리했음을 운영체제에 알립니다.
    } // switch 블록 끝

    // 우리가 switch문에서 명시적으로 처리하지 않은 나머지 모든 윈도우 메시지들을
    // 운영체제의 기본 윈도우 프로시저가 대신 처리하도록 넘겨줍니다. (예: 창 크기 조절, 창 이동 등)
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
} // WindowProc 함수의 끝