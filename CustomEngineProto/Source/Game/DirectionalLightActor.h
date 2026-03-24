#pragma once // 헤더 파일 중복 포함 방지입니다.
#include "Framework/Core/Actor.h" // 부모 액터 클래스 포함입니다.
#include <memory> // 스마트 포인터를 위해 포함합니다.

class LightComponent; // 조명 부품을 쓰기 위해 전방 선언합니다.

//  게임 세상(Scene)에 배치되어 전체를 비추는 '태양' 역할을 하는 액터입니다! 
class DirectionalLightActor : public Actor // Actor를 상속받습니다.
{ // 클래스 블록 시작
public: // 퍼블릭
    DirectionalLightActor(); // 생성자입니다.
    virtual void Update(float deltaTime) override; // 매 프레임 태양의 일주 운동(낮/밤)을 연산할 함수입니다.

private: // 프라이빗
    std::shared_ptr<LightComponent> mLightComp; //   내 몸에 부착하여 빛을 뿜게 만들 조명 컴포넌트 부품입니다.
}; // 클래스 블록 끝