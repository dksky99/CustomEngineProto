#include "InputManager.h" // 자신의 헤더 파일을 포함합니다.

InputManager& InputManager::GetInstance() // 싱글톤 인스턴스를 반환하는 함수입니다.
{ // 함수 시작
    static InputManager instance; // 메모리에 단 한 번만 생성되는 정적(static) 객체입니다.
    return instance; // 생성된 유일한 객체의 참조(Reference)를 반환합니다.
} // 함수 끝

void InputManager::Update() // 매 프레임 엔진 메인 루프에서 호출될 업데이트 함수입니다.
{ // 함수 시작
    POINT currentMousePos; // 현재 마우스 커서의 위치를 담을 구조체입니다.
    GetCursorPos(&currentMousePos); // 윈도우 운영체제에 물어봐서 모니터 상의 현재 마우스 좌표를 가져옵니다.

    // 현재 위치에서 이전 프레임의 위치를 빼서 순수한 '이동량(Delta)'만 계산합니다.
    mMouseDelta.x = currentMousePos.x - mLastMousePos.x; // X축(좌우) 마우스 이동량
    mMouseDelta.y = currentMousePos.y - mLastMousePos.y; // Y축(상하) 마우스 이동량

    mLastMousePos = currentMousePos; // 다음 프레임 계산을 위해 현재 위치를 과거 위치로 갱신해 둡니다.
} // 함수 끝

bool InputManager::IsKeyPress(int vKey) const // 키보드 눌림 여부 확인 함수입니다.
{ // 함수 시작
    // GetAsyncKeyState는 키보드의 하드웨어적 상태를 실시간으로 즉시 확인합니다. 최상위 비트(0x8000)가 1이면 눌린 상태입니다.
    return (GetAsyncKeyState(vKey) & 0x8000) != 0;
} // 함수 끝

bool InputManager::IsMouseRButtonDown() const // 마우스 우클릭 여부 확인 함수입니다.
{ // 함수 시작
    // VK_RBUTTON(마우스 우클릭)의 하드웨어 눌림 상태를 확인하여 반환합니다.
    return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
} // 함수 끝

POINT InputManager::GetMouseDelta() const
{
    return mMouseDelta;
}
