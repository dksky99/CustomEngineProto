#pragma once // 헤더 파일이 중복으로 포함되는 것을 방지합니다.
#include <windows.h> // 키보드 및 마우스 입력을 처리하는 윈도우 API를 사용하기 위해 포함합니다.

// 게임 엔진 내의 모든 사용자의 입력을 중앙에서 통제하는 관리자 클래스입니다. 
class InputManager // InputManager 클래스 선언입니다.
{ // 클래스 블록 시작
public: // 누구나 접근 가능합니다.
    static InputManager& GetInstance(); // 이 객체는 프로그램 전체에서 단 1개만 존재해야 하므로 싱글톤(Singleton) 패턴으로 접근합니다.

    void Update(); // 매 프레임 호출되어 마우스 이동량과 키보드 상태를 최신화합니다.

    bool IsKeyPress(int vKey) const; // 특정 키보드 키(예: 'W', 'A')가 눌렸는지 확인하는 함수입니다.
    bool IsMouseRButtonDown() const; // 마우스 오른쪽 버튼이 꾹 눌려있는 상태인지 확인하는 함수입니다.
    POINT GetMouseDelta() const; // 이번 프레임에 마우스가 화면상에서 얼만큼 이동했는지(Delta) X, Y 픽셀 값을 반환합니다.

private: // 외부 생성 금지
    InputManager() = default; // 싱글톤 패턴을 위해 생성자를 함부로 호출하지 못하게 숨깁니다.

    POINT mLastMousePos = { 0, 0 }; // 이전 프레임의 마우스 X, Y 위치를 기억해둘 변수입니다.
    POINT mMouseDelta = { 0, 0 }; // 방금 전 프레임 대비 마우스가 얼마나 움직였는지 계산된 이동량입니다.
}; // 클래스 블록 끝