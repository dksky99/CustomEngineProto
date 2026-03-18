#include "Actor.h" // Actor 헤더 파일을 포함합니다.

Actor::Actor() // 액터 생성자입니다.
{ // 생성자 시작입니다.
} // 생성자 끝입니다.

Actor::~Actor() // 액터 소멸자입니다.
{ // 소멸자 시작입니다.
} // 소멸자 끝입니다.

void Actor::Update(float deltaTime) // 액터의 매 프레임 갱신 함수입니다.
{ // 함수 시작입니다.
    for (auto& comp : mComponents) // 내가 장착한 모든 컴포넌트 부품들을 순회합니다.
    { // 루프 시작입니다.
        comp->Update(deltaTime); // 각 부품들에게도 1프레임어치 일을 하라고 지시합니다.
    } // 루프 끝입니다.
} // 함수 끝입니다.

void Actor::AddComponent(std::shared_ptr<Component> comp) // 컴포넌트 장착 함수 구현부입니다.
{ // 함수 시작입니다.
    comp->SetOwner(this); // 끼워 넣을 부품에게 "이제부터 네 주인은 나야"라고 알려줍니다.
    mComponents.push_back(comp); // 내 부품 목록 배열에 안전하게 추가합니다.
} // 함수 끝입니다.