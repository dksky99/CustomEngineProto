#include "Scene.h" // 구현을 위해 Scene 헤더 파일을 불러옵니다.

Scene::Scene() // 씬 생성자의 구현부입니다.
{ // 생성자 시작입니다.
    // 씬 초기화 로직이 들어가는 곳입니다. 배열은 자동으로 빈 상태로 생성됩니다.
} // 생성자 끝입니다.

Scene::~Scene() // 씬 소멸자의 구현부입니다.
{ // 소멸자 시작입니다.
    // 스마트 포인터(std::shared_ptr)를 사용했으므로, 씬이 파괴될 때 mActors 내부의 메모리도 자동으로 안전하게 해제됩니다.
} // 소멸자 끝입니다.

void Scene::Update(float deltaTime) // 씬 내부의 모든 액터 로직을 갱신하는 함수입니다.
{ // 함수 시작입니다.
    // 씬 배열(mActors) 안에 들어있는 모든 액터들을 처음부터 끝까지 하나씩 순회(Loop)합니다.
    for (auto& actor : mActors)
    { // 루프 시작입니다.
        // 각 액터의 내부에 구현된 행동 강령(Update)을 실행하라고 명령을 내립니다.
        actor->Update(deltaTime);
    } // 루프 끝입니다.
} // 함수 끝입니다.

void Scene::AddActor(std::shared_ptr<Actor> actor) // 씬에 새 액터를 추가하는 함수입니다.
{ // 함수 시작입니다.
    // 넘겨받은 액터의 스마트 포인터를 mActors 배열의 가장 마지막 칸에 안전하게 밀어 넣습니다.
    mActors.push_back(actor);
} // 함수 끝입니다.