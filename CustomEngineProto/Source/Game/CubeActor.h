#pragma once
#include "Framework/Core/Actor.h"

//  [신규 파일] 프레임워크를 활용해 개발자가 직접 만든 첫 번째 '게임 콘텐츠' 클래스입니다. 
class CubeActor : public Actor // 엔진의 기본 Actor 클래스를 상속받아 CubeActor를 만듭니다.
{ // 클래스 블록 시작입니다.
public: // 외부에서 접근할 수 있도록 퍼블릭으로 엽니다.
    int CubeIndex = 0; // 각 큐브마다 다른 회전 속도를 주기 위한 고유 식별 번호 변수입니다.

    // 부모(Actor)가 만들어둔 빈 업데이트 함수를 덮어쓰기(override)하여, 이 큐브만의 고유한 행동을 정의합니다.
    virtual void Update(float deltaTime) override;
}; // 클래스 블록 끝입니다.
