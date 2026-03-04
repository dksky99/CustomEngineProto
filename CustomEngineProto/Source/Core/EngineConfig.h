#pragma once // 이 헤더 파일이 여러 곳에서 중복으로 포함(Include)되는 것을 방지하여 컴파일 오류를 막아줍니다.

#include <windows.h> // 윈도우 관련 기본 타입(HWND, HINSTANCE 등)을 사용하기 위해 포함합니다.

// 엔진의 전역적인 설정값들을 한곳에 모아 관리하기 위한 구조체입니다.
struct EngineConfig
{ // EngineConfig 구조체의 시작입니다.

    // --- 윈도우 및 디스플레이 설정 ---
    int WindowWidth = 1280; // 게임 윈도우 창의 기본 너비(픽셀 단위)를 1280으로 설정합니다.
    int WindowHeight = 720; // 게임 윈도우 창의 기본 높이(픽셀 단위)를 720으로 설정합니다.
    bool bWindowed = true; // 창 모드로 실행할지(true), 전체 화면으로 실행할지(false)를 결정하는 플래그입니다.
    const wchar_t* WindowTitle = L"My Custom DX12 Engine"; // 윈도우 창 상단 타이틀 바에 표시될 엔진의 이름입니다.

    // --- 렌더링 파이프라인 설정 ---
    bool bEnableVSync = true; // 수직 동기화(VSync)를 켜서 모니터 주사율에 프레임을 맞출지 여부입니다. (화면 찢어짐 방지)
    int FrameBufferCount = 2; // 스왑 체인에서 사용할 버퍼의 개수입니다. (2 = 더블 버퍼링, 3 = 트리플 버퍼링)

    // --- 시스템 핸들 (실행 중 운영체제로부터 받아오는 값들) ---
    HWND MainWindowHandle = nullptr; // 화면을 출력할 실제 윈도우 창의 핸들(ID)을 저장할 포인터입니다.
    HINSTANCE AppInstance = nullptr; // 현재 실행 중인 프로그램(엔진)의 프로세스 인스턴스 핸들을 저장합니다.

}; // EngineConfig 구조체의 끝입니다.