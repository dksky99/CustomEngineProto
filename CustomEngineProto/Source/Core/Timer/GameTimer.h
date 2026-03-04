#pragma once // 이 헤더 파일이 여러 번 중복으로 포함(Include)되는 것을 방지합니다.
#include <windows.h> // 고해상도 타이머 함수(QueryPerformanceCounter 등)를 사용하기 위해 Windows API 헤더를 포함합니다.

class GameTimer// 게임 내 시간 계산과 전반적인 시간 관리를 담당하는 클래스입니다.
{ // GameTimer 클래스 정의 시작
public: // 외부 시스템(게임 루프 등)에서 접근 가능한 퍼블릭 멤버 함수들을 선언합니다.
    GameTimer(); // GameTimer 클래스의 인스턴스가 생성될 때 호출되는 기본 생성자입니다.

    float DeltaTime() const; // 이전 프레임과 현재 프레임 사이의 흘러간 시간(초 단위)을 반환하는 함수입니다.

    void Reset(); // 게임 루프가 시작되기 직전에 타이머를 초기 상태로 세팅하는 함수입니다.
    void Tick(); // 매 프레임(루프)마다 호출되어 현재 시간을 갱신하고 델타 타임을 계산하는 함수입니다.

private: // 클래스 내부에서만 값을 읽고 쓸 수 있는 프라이빗 멤버 변수들을 선언합니다.
    double mSecondsPerCount; // 하드웨어 타이머의 1카운트가 실제 몇 초에 해당하는지(주기)를 저장합니다.
    double mDeltaTime; // 틱(Tick)마다 계산된 프레임 간의 시간 차이(DeltaTime)를 보관합니다.

    __int64 mBaseTime; // 타이머가 리셋(Reset)된 시점의 기준 하드웨어 카운트 값을 저장합니다.
    __int64 mPausedTime; // 타이머가 일시 정지되었던 총 누적 카운트 값을 저장합니다. (추후 일시정지 기능 확장용)
    __int64 mStopTime; // 타이머가 정지된 바로 그 시점의 카운트 값을 임시로 보관합니다.
    __int64 mPrevTime; // 직전 프레임에서 측정되었던 하드웨어 카운트 값을 저장합니다.
    __int64 mCurrTime; // 현재 프레임에서 새롭게 측정한 하드웨어 카운트 값을 저장합니다.

    bool mStopped; // 현재 타이머가 일시 정지 상태인지 여부를 나타내는 불리언 플래그입니다.
}; // GameTimer 클래스 정의 끝