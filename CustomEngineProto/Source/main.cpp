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
//  태양에 조명 부품을 부착하기 위해 헤더를 포함합니다! 
#include "Framework/Components/LightComponent.h"


#include "Game/CubeActor.h"

//  액터에게 외형 부품을 달아주기 위해 메시 컴포넌트 헤더를 포함합니다. 
#include "Framework/Components/MeshComponent.h" 

#include "Framework/Components/CameraComponent.h" 

//   [추가점] 흩어져 있던 마우스/키보드 입력을 처리할 중앙 통제실 헤더를 포함합니다!  
#include "Framework/Core/InputManager.h" 

//  씬에 배치할 태양(Directional Light) 액터의 헤더를 포함합니다! 
#include "Game/DirectionalLightActor.h" 
// 머티리얼을 직접 생성하기 위해 헤더를 포함합니다.
#include "Graphics/Resources/Material.h"


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


    Scene scene; // 월드(맵)를 담당할 씬 객체를 메모리에 올립니다.
    


    //  탈출 문자가 발생하지 않도록 역슬래시를 두 번(\\) 적거나 슬래시(/)를 사용해야 합니다! 
   // 회원님께서 알려주신 정확한 절대 경로를 안전한 문법으로 덮어씌웠습니다.
    std::shared_ptr<Texture> sunTex = renderer.LoadTexture(L"D:\\Git\\CustomEngineProto\\CustomEngineProto\\Resources\\Textures\\T_Sun.jpg");
    std::shared_ptr<Texture> earthTex = renderer.LoadTexture(L"D:\\Git\\CustomEngineProto\\CustomEngineProto\\Resources\\Textures\\T_Earth.jpg");
    std::shared_ptr<Texture> moonTex = renderer.LoadTexture(L"D:\\Git\\CustomEngineProto\\CustomEngineProto\\Resources\\Textures\\T_Moon.jpg");
    std::shared_ptr<Texture> floorTex = renderer.LoadTexture(L"D:\\Git\\CustomEngineProto\\CustomEngineProto\\Resources\\Textures\\T_Floor.jpg");

    //  다운로드하신 노멀맵(Normal Map)을 로드합니다! 
   // 구글에 "Earth normal map", "Moon normal map" 등을 검색하셔서 다운받고 폴더에 넣어주세요! 
   // 파일이 없으면 가짜 평면이 씌워지므로 에러가 나진 않지만 입체감 마법은 구경할 수 없습니다!
    std::shared_ptr<Texture> earthNormal = renderer.LoadTexture(L"D:\\Git\\CustomEngineProto\\CustomEngineProto\\Resources\\Textures\\N_Earth.jpg");
    std::shared_ptr<Texture> moonNormal = renderer.LoadTexture(L"D:\\Git\\CustomEngineProto\\CustomEngineProto\\Resources\\Textures\\N_Moon.jpg");
    std::shared_ptr<Texture> floorNormal = renderer.LoadTexture(L"D:\\Git\\CustomEngineProto\\CustomEngineProto\\Resources\\Textures\\N_Floor.jpg");

    //  화면 중앙에 띄울 '십자선(Crosshair)' 이미지를 불러옵니다! 
    // 인터넷에서 투명한 배경을 가진 PNG 형태의 십자선, 크로스헤어 이미지를 다운로드하여 넣어주세요.
    std::shared_ptr<Texture> crosshairTex = renderer.LoadTexture(L"Resources/Textures/T_Crosshair.png");

    //  태양, 지구, 달, 바닥을 위한 각자의 고유한 재질(Material)을 만듭니다! 

    std::shared_ptr<Material> sunMat = std::make_shared<Material>();
    sunMat->DiffuseMap = sunTex; // 태양 텍스처 장착!
    sunMat->DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    sunMat->Emissive = { 2.0f, 1.0f, 0.0f };

    std::shared_ptr<Material> earthMat = std::make_shared<Material>();
    earthMat->DiffuseMap = earthTex; // 지구 텍스처 장착!
    earthMat->NormalMap = earthNormal; // 지구에 대륙의 굴곡을 입힙니다!
    earthMat->DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    earthMat->Emissive = { 0.0f, 0.0f, 0.0f };

    std::shared_ptr<Material> moonMat = std::make_shared<Material>();
    moonMat->DiffuseMap = moonTex; // 달 텍스처 장착!
    moonMat->NormalMap = moonNormal; // 달에 수만 개의 분화구를 깊게 파냅니다!
    moonMat->DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    moonMat->Emissive = { 0.0f, 0.0f, 0.0f };

    std::shared_ptr<Material> floorMat = std::make_shared<Material>();
    floorMat->DiffuseMap = floorTex; // 바닥 텍스처 장착!
    floorMat->NormalMap = floorNormal; // 밋밋한 바닥에 오돌토돌한 돌 재질을 살려줍니다!
    floorMat->DiffuseAlbedo = { 0.7f, 0.7f, 0.7f, 1.0f };
    floorMat->Emissive = { 0.0f, 0.0f, 0.0f };





        // 1. 거대한 중앙의 태양(Sun)을 만듭니다.
    std::shared_ptr<Actor> sunActor = std::make_shared<Actor>(); // 태양 액터를 만듭니다.
    sunActor->GetTransform()->Scale = { 2.0f, 2.0f, 2.0f }; // 크기를 2배로 키웁니다.
    std::shared_ptr<MeshComponent> sunMesh = std::make_shared<MeshComponent>(); // 메시 부품을 만듭니다.
    sunMesh->SetMesh(renderer.GetDefaultMesh()); // 껍데기를 씌웁니다.
    sunMesh->SetMaterial(sunMat); // 재질을 씌웁니다.
    sunActor->AddComponent(sunMesh); // 몸통에 조립합니다.

    //  텅 빈 모형이었던 태양의 몸통에 사방으로 퍼지는 점광원(Point Light) 부품을 장착합니다! 
    std::shared_ptr<LightComponent> sunPointLight = std::make_shared<LightComponent>();
    sunPointLight->Type = ELightType::Point; // 나는 점광원이다!
    sunPointLight->LightColor = { 2.0f, 1.8f, 0.8f, 1.0f }; // 강렬한 태양빛 (주황/노랑)
    sunPointLight->FalloffStart = 5.0f;  // 5미터까지는 100% 눈부시게 밝습니다.
    sunPointLight->FalloffEnd = 25.0f;   // 25미터를 넘어가면 빛이 완전히 사라집니다.
    sunActor->AddComponent(sunPointLight); // 태양에 부착!
    // ---------------------------------------------------------------------- 

    scene.AddActor(sunActor); // 씬에 올립니다.

    // 2. 태양을 공전하는 지구(Earth)를 만듭니다.
    std::shared_ptr<Actor> earthActor = std::make_shared<Actor>(); // 지구 액터를 만듭니다.
    earthActor->GetTransform()->Scale = { 0.5f, 0.5f, 0.5f }; // 크기를 0.5배로 줄입니다.
    earthActor->GetTransform()->Position = { 5.0f, 0.0f, 0.0f }; //   중요: 태양의 중심에서부터 X축으로 5.0만큼 거리를 벌려줍니다! (공전 반경)  
    std::shared_ptr<MeshComponent> earthMesh = std::make_shared<MeshComponent>(); // 메시 부품을 만듭니다.
    earthMesh->SetMesh(renderer.GetDefaultMesh()); // 껍데기를 씌웁니다.
    earthMesh->SetMaterial(earthMat); // 재질을 씌웁니다.
    earthActor->AddComponent(earthMesh); // 몸통에 조립합니다.

    // ★   [계층 구조 연결 1] 지구의 부모를 태양으로 설정합니다!   ★
    earthActor->GetTransform()->SetParent(sunActor->GetTransform());
    scene.AddActor(earthActor); // 씬에 올립니다.

    // 3. 지구를 공전하는 달(Moon)을 만듭니다.
    std::shared_ptr<Actor> moonActor = std::make_shared<Actor>(); // 달 액터를 만듭니다.
    moonActor->GetTransform()->Scale = { 0.2f, 0.2f, 0.2f }; // 크기를 0.2배로 아주 작게 만듭니다.
    moonActor->GetTransform()->Position = { 2.0f, 0.0f, 0.0f }; //   중요: 지구의 중심에서부터 X축으로 2.0만큼 거리를 벌려줍니다!  
    std::shared_ptr<MeshComponent> moonMesh = std::make_shared<MeshComponent>(); // 메시 부품을 만듭니다.
    moonMesh->SetMesh(renderer.GetDefaultMesh()); // 껍데기를 씌웁니다.
    moonMesh->SetMaterial(moonMat); // 재질을 씌웁니다.
    moonActor->AddComponent(moonMesh); // 몸통에 조립합니다.

    // ★   [계층 구조 연결 2] 달의 부모를 지구로 설정합니다!   ★
    moonActor->GetTransform()->SetParent(earthActor->GetTransform());
    scene.AddActor(moonActor); // 씬에 올립니다.

    //   [추가점] 큐브들의 그림자를 받아낼 '거대한 바닥(Floor)'을 하나 추가합니다!  
    std::shared_ptr<CubeActor> floor = std::make_shared<CubeActor>();
    floor->GetTransform()->Position = { 0.0f, -2.0f, 0.0f }; // 100개 큐브들 발밑에 배치
    floor->GetTransform()->Scale = { 30.0f, 1.0f, 30.0f }; // 아주 넓고 납작하게 폅니다.

    std::shared_ptr<MeshComponent> floorMesh = std::make_shared<MeshComponent>();
    floorMesh->SetMesh(renderer.GetDefaultMesh());
    floorMesh->SetMaterial(floorMat);
    floor->AddComponent(floorMesh);
    scene.AddActor(floor);

    // -----------------------------------------------------------------------------------
    //  화려한 다중 점광원 3형제를 새롭게 스폰합니다! 
    // 1번 빛: 붉은색 사이렌 조명 액터를 만듭니다.
    std::shared_ptr<Actor> redLightActor = std::make_shared<Actor>();
    std::shared_ptr<LightComponent> redLight = std::make_shared<LightComponent>(); // 빛을 뿜는 부품을 만듭니다.
    redLight->Type = ELightType::Point; // 방향광이 아니라 '점광원'이라고 선언합니다. (회원님의 enum을 따릅니다)
    redLight->LightColor = { 5.0f, 0.2f, 0.2f, 1.0f }; // 아주 강렬한 붉은빛으로 설정합니다. (PBR 반응 극대화)
    redLight->FalloffStart = 2.0f; // 2미터까지는 눈부시게 밝습니다.
    redLight->FalloffEnd = 15.0f; // 15미터가 넘어가면 붉은빛이 소멸합니다.
    redLightActor->AddComponent(redLight); // 부품을 몸통에 장착합니다!
    scene.AddActor(redLightActor); // 씬에 배치합니다.

    // 2번 빛: 푸른색 코어 조명을 만듭니다.
    std::shared_ptr<Actor> blueLightActor = std::make_shared<Actor>();
    std::shared_ptr<LightComponent> blueLight = std::make_shared<LightComponent>();
    blueLight->Type = ELightType::Point;
    blueLight->LightColor = { 0.2f, 0.2f, 5.0f, 1.0f }; // 강렬한 푸른빛!
    blueLight->FalloffStart = 2.0f;
    blueLight->FalloffEnd = 15.0f;
    blueLightActor->AddComponent(blueLight);
    scene.AddActor(blueLightActor);

    // 3번 빛: 초록색 반딧불 조명을 만듭니다.
    std::shared_ptr<Actor> greenLightActor = std::make_shared<Actor>();
    std::shared_ptr<LightComponent> greenLight = std::make_shared<LightComponent>();
    greenLight->Type = ELightType::Point;
    greenLight->LightColor = { 0.2f, 5.0f, 0.2f, 1.0f }; // 강렬한 초록빛!
    greenLight->FalloffStart = 2.0f;
    greenLight->FalloffEnd = 15.0f;
    greenLightActor->AddComponent(greenLight);
    scene.AddActor(greenLightActor);
    // -----------------------------------------------------------------------------------


    
    
    
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

     // --- 태양빛(Directional Light) 세팅 ---
    std::shared_ptr<DirectionalLightActor> worldLight = std::make_shared<DirectionalLightActor>(); // 조명 액터 생성
    //  태양이 진짜 점광원 역할을 맡았으므로, 세상을 밝히던 메인 방향광은 '어둡고 푸른 우주 배경빛'으로 용도를 변경합니다! 
    auto dirLightComp = worldLight->GetComponent<LightComponent>();
    if (dirLightComp)
    {
        dirLightComp->LightColor = { 0.1f, 0.1f, 0.25f, 1.0f }; // 어둡고 차가운 남색 계열로 덮어씌웁니다.
    }


    scene.AddActor(worldLight); // 씬에 조명 등록
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


            //   [우주 시뮬레이션] 각 행성을 제자리에서 자전(회전)시킵니다!  
           // 태양(Sun)을 자전시킵니다. (부모인 태양이 돌면 자식인 지구가 우주 공간을 공전하게 됩니다!)
            sunActor->GetTransform()->Rotation.y += timer.DeltaTime() * 0.5f;

            // 지구(Earth)를 자전시킵니다. (지구가 돌면 자식인 달이 지구를 중심으로 공전하게 됩니다!)
            earthActor->GetTransform()->Rotation.y += timer.DeltaTime() * 2.0f;

            // 달(Moon)도 자기 자신을 빠르게 자전시킵니다.
            moonActor->GetTransform()->Rotation.y += timer.DeltaTime() * 5.0f;
            // -----------------------------------------------------------------------------------

             //  3가지 색상의 점광원을 시간의 흐름에 따라 어지럽게 날아다니도록 만듭니다! 
            float t = timer.TotalTime(); // 게임이 켜진 후 누적된 시간(초)을 가져옵니다.

            // 붉은 빛은 반경 8m 크기로 바닥(-1.5f 높이) 근처를 예쁘게 둥글게 공전합니다.
            redLightActor->GetTransform()->Position = { cosf(t * 1.5f) * 8.0f, -1.5f, sinf(t * 1.5f) * 8.0f };

            // 푸른 빛은 반경 5m 크기로 붉은 빛과 '반대 방향'(-t)으로 궤도를 교차하며 돕니다.
            blueLightActor->GetTransform()->Position = { cosf(-t * 2.0f) * 5.0f, -1.5f, sinf(-t * 2.0f) * 5.0f };

            // 초록 빛은 X축과 Z축의 주기를 다르게 주어, 8자 모양(리사주 도형)으로 어지럽게 횡단하게 만듭니다!
            greenLightActor->GetTransform()->Position = { sinf(t * 3.0f) * 7.0f, -1.0f, sinf(t * 1.5f) * 7.0f };



            //   2단계: 씬 갱신! (이때 카메라 액터 안에 달린 CameraComponent도 씬을 통해 자동으로 Update 됩니다!)  
            scene.Update(timer.DeltaTime());

            //  매 프레임 화면 정중앙에 십자선을 그려달라고 렌더러에게 요청합니다! 
            if (crosshairTex) // 텍스처 로딩에 성공했다면
            {
                float uiWidth = 50.0f;  // 십자선 가로 크기 50픽셀
                float uiHeight = 50.0f; // 십자선 세로 크기 50픽셀
                // 모니터 한가운데 좌표를 구합니다. (좌상단이 0,0 기준이므로 너비의 절반에서 크기의 절반을 뺍니다)
                float uiX = (config.WindowWidth / 2.0f) - (uiWidth / 2.0f);
                float uiY = (config.WindowHeight / 2.0f) - (uiHeight / 2.0f);

                // 렌더러의 UI 대기열에 집어넣습니다!
                renderer.DrawUI(crosshairTex, uiX, uiY, uiWidth, uiHeight);
            }





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