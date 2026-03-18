#pragma once // 헤더 중복 포함 방지입니다.

#include <vector> // 여러 개의 액터를 순차적으로 보관할 가변 배열(Vector)을 사용하기 위함입니다.
#include <memory> // 메모리 누수를 막기 위해 액터 관리에 스마트 포인터를 사용하기 위함입니다.
#include "Actor.h" // 씬 안에 배치할 액터 클래스 헤더를 포함합니다.

//  거대한 게임 세상(맵)을 대변하며, 그 안의 모든 액터들을 총괄 관리하는 씬(Scene) 클래스입니다. 
class Scene // Scene 클래스 선언입니다. (언리얼 엔진의 UWorld와 유사한 역할입니다)
{ // 클래스 내부 블록 시작입니다.
public: // 퍼블릭 접근 제어자입니다.
    Scene(); // 씬이 처음 로드될 때 호출되는 생성자입니다.
    ~Scene(); // 씬이 닫힐 때 호출되는 소멸자입니다.

    // 씬에 존재하는 모든 액터들의 Update 함수를 일괄적으로 순회하며 호출해 주는 중앙 통제 함수입니다.
    void Update(float deltaTime);

    // 외부에서 씬(게임 세상)에 새로운 물체(액터)를 스폰(배치)할 때 사용하는 함수입니다.
    void AddActor(std::shared_ptr<Actor> actor);

    // 렌더러가 화면을 그리기 위해, 현재 씬에 소속된 모든 액터들의 명부(배열)를 읽기 전용으로 요청할 때 씁니다.
    const std::vector<std::shared_ptr<Actor>>& GetActors() const { return mActors; }

private: // 외부에서 함부로 액터 목록을 조작하지 못하게 막는 프라이빗 영역입니다.
    std::vector<std::shared_ptr<Actor>> mActors; // 이 씬에 배치된 모든 액터들의 스마트 포인터를 담아두는 거대한 배열 컨테이너입니다.
}; // 클래스 블록 끝입니다.