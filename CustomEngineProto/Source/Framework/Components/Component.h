#pragma once // 헤더 파일이 중복으로 포함되는 것을 방지합니다.

class Actor; // 컴포넌트가 자신의 주인을 알 수 있도록 Actor 클래스를 전방 선언합니다.

// 모든 컴포넌트(부품)의 뼈대가 되는 최상위 부모 클래스입니다.
class Component // Component 클래스 선언입니다.
{ // 클래스 블록 시작입니다.
public: // 누구나 접근 가능합니다.
    virtual ~Component() = default; // 다형성을 위해 가상 소멸자를 기본값으로 생성합니다.

    virtual void Update(float deltaTime) {} // 매 프레임 부품 스스로 작동할 수 있도록 가상 업데이트 함수를 뚫어둡니다.

    Actor* GetOwner() const { return mOwner; } // 이 부품을 장착한 주인(액터)의 포인터를 반환합니다.
    void SetOwner(Actor* owner) { mOwner = owner; } // 이 부품의 주인을 설정합니다. (Actor::AddComponent에서 호출됨)

protected: // 자식 컴포넌트들만 접근 가능한 영역입니다.
    Actor* mOwner = nullptr; // 나를 장착하고 있는 주인 액터의 메모리 주소입니다.
}; // 클래스 블록 끝입니다.