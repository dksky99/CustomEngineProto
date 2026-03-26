#include "CubeActor.h" // 자신이 선언된 헤더 파일을 포함합니다.

//  정육면체 액터가 매 프레임 해야 할 행동(로직)을 구현하는 곳입니다. 
void CubeActor::Update(float deltaTime) // 매 프레임 호출되는 업데이트 함수의 구현부입니다.
{ // 함수 블록 시작입니다.
    // 부모 클래스(Actor)가 가지고 있는 트랜스폼(mTransform) 멤버 변수에 접근하여 X축 회전값을 갱신합니다.
    // deltaTime을 곱해 프레임 속도에 상관없이 일정한 속도로 돌게 하며, CubeIndex를 이용해 큐브마다 미세하게 다른 속도를 줍니다.
    //mTransform.Rotation.x += deltaTime * (1.0f + CubeIndex * 0.01f);

    // Y축 회전값도 갱신하여 큐브가 대각선으로 빙글빙글 돌도록 만듭니다.
    //mTransform.Rotation.y += deltaTime * (0.5f + CubeIndex * 0.02f);
} // 함수 블록 끝입니다.