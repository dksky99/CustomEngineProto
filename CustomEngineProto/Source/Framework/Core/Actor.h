#pragma once // 헤더 중복 포함 방지 지시어입니다.

#include <vector> // 컴포넌트들을 담을 가변 배열을 위해 포함합니다.
#include <memory> // 스마트 포인터를 위해 포함합니다.
#include "Transform.h" // 트랜스폼 헤더를 포함합니다.
#include "Framework/Components/Component.h" // 컴포넌트 부모 헤더를 포함합니다.

// 액터가 이제 여러 개의 부품(컴포넌트)을 장착할 수 있는 레고 판이 되었습니다! 
class Actor // Actor 클래스 선언입니다.
{ // 클래스 내부 블록 시작입니다.
public: // 퍼블릭 영역입니다.
    Actor(); // 생성자입니다.
    virtual ~Actor(); // 가상 소멸자입니다.

    virtual void Update(float deltaTime); // 매 프레임 액터와 컴포넌트들을 갱신하는 함수입니다.

    Transform* GetTransform() { return &mTransform; } // 트랜스폼 객체의 주소를 반환합니다.

    void AddComponent(std::shared_ptr<Component> comp); // 액터에 새로운 부품(컴포넌트)을 끼워 넣는 함수입니다.

    template<typename T> // 어떤 타입의 컴포넌트든 찾을 수 있도록 템플릿 기법을 사용합니다.
    std::shared_ptr<T> GetComponent() // 특정 타입의 컴포넌트를 찾아 반환하는 함수입니다.
    { // 템플릿 함수 블록 시작입니다.
        for (auto& comp : mComponents) // 내가 장착한 모든 컴포넌트를 순회합니다.
        { // 루프 시작입니다.
            std::shared_ptr<T> target = std::dynamic_pointer_cast<T>(comp); // 현재 컴포넌트가 찾는 타입(T)이 맞는지 안전하게 캐스팅해 봅니다.
            if (target) return target; // 캐스팅에 성공했다면(타입이 일치한다면) 그 부품을 반환합니다.
        } // 루프 끝입니다.
        return nullptr; // 끝까지 찾았는데 없다면 빈 포인터를 반환합니다.
    } // 템플릿 함수 블록 끝입니다.

protected: // 프로텍티드 영역입니다.
    Transform mTransform; // 액터의 위치, 회전, 크기 정보입니다.
    std::vector<std::shared_ptr<Component>> mComponents; // 액터가 장착한 모든 부품(컴포넌트)들의 목록입니다.
}; // 클래스 블록 끝입니다.