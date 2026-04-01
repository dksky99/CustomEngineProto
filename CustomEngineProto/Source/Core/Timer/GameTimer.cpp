#include "GameTimer.h"// 앞서 선언한 GameTimer 클래스의 구조를 알기 위해 헤더를 포함합니다.

GameTimer::GameTimer() // GameTimer 생성자의 구현부입니다.
    : mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0), // 콜론(:) 초기화 리스트를 사용해 변수들을 0이나 초기 상태로 세팅합니다.
    mPausedTime(0), mPrevTime(0), mCurrTime(0), mTotalTime(0), mStopped(false) // 일시정지 누적 시간, 이전/현재 시간, 정지 플래그를 모두 초기화합니다.
{ // 생성자 본문 시작
    __int64 countsPerSec; // 1초당 하드웨어 타이머가 몇 번 카운트되는지(주파수)를 담을 변수를 선언합니다.
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec); // 운영체제에 고해상도 타이머의 주파수를 질의하여 변수에 채워 넣습니다.
    mSecondsPerCount = 1.0 / (double)countsPerSec; // 1카운트당 몇 초인지 구하기 위해 주파수의 역수(1 / 주파수)를 계산하여 저장합니다.
} // 생성자 본문 끝

float GameTimer::DeltaTime() const // 델타 타임을 반환하는 함수의 구현부입니다.
{ // DeltaTime 함수 시작
    return (float)mDeltaTime; // 내부에서 배정밀도(double)로 정밀하게 계산된 델타 타임을 단정밀도(float)로 형변환하여 반환합니다.
} // DeltaTime 함수 끝

void GameTimer::Reset() // 타이머 초기화 함수의 구현부입니다. 게임 루프 진입 직전에 한 번 호출해야 합니다.
{ // Reset 함수 시작
    __int64 currTime; // 현재 측정할 하드웨어 카운트 값을 담을 지역 변수를 선언합니다.
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime); // 고해상도 타이머의 현재 카운트 값을 운영체제로부터 가져옵니다.

    mBaseTime = currTime; // 가져온 현재 시간을 전체 게임 시간의 시작점인 기준 시간(BaseTime)으로 기록합니다.
    mPrevTime = currTime; // 아직 프레임이 진행되지 않았으므로, 이전 시간(PrevTime)도 현재 시간과 동일하게 맞춥니다.
    mStopTime = 0; // 정지된 적이 없으므로 정지 시간 기록을 0으로 초기화합니다.
    mStopped = false; // 타이머가 정상적으로 동작 중임을 알리기 위해 정지 플래그를 false로 설정합니다.
} // Reset 함수 끝

void GameTimer::Tick() // 매 프레임 호출되어 시간을 갱신하는 함수의 구현부입니다.
{ // Tick 함수 시작
    if (mStopped) // 만약 타이머가 일시 정지된 상태라면,
    { // if 조건 블록 시작
        mDeltaTime = 0.0; // 시간의 흐름이 없어야 하므로 델타 타임을 0으로 설정합니다.
        return; // 이후의 시간 계산 로직을 건너뛰고 함수를 즉시 빠져나갑니다.
    } // if 조건 블록 끝

    __int64 currTime; // 현재 하드웨어 카운트를 담을 지역 변수를 선언합니다.
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime); // 고해상도 타이머의 가장 최신 카운트 값을 가져옵니다.
    mCurrTime = currTime; // 가져온 값을 클래스의 멤버 변수인 현재 시간(mCurrTime)에 저장합니다.

    // (현재 시간 - 이전 시간)을 빼서 카운트 차이를 구하고, 거기에 1카운트당 초(SecondsPerCount)를 곱해 실제 흐른 시간(초)을 구합니다.
    mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;

    mPrevTime = mCurrTime; // 다음 프레임의 계산을 위해, 방금 측정한 현재 시간을 이전 시간(mPrevTime) 변수에 덮어씁니다.
    mTotalTime += mDeltaTime;
    if (mDeltaTime < 0.0) // 프로세서 절전 모드 전환이나 멀티코어 버그 등으로 델타 타임이 음수가 나오는 희귀한 예외 상황을 검사합니다.
    { // 예외 처리 블록 시작
        mDeltaTime = 0.0; // 물리엔진 등이 역주행하는 치명적인 오류를 막기 위해 델타 타임을 0으로 강제 보정합니다.
    } // 예외 처리 블록 끝
} // Tick 함수 끝

float GameTimer::TotalTime() const
{

    return (float)mTotalTime;
}
