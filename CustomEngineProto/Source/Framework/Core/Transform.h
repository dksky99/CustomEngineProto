#pragma once // 헤더 파일이 중복으로 포함되는 것을 방지합니다.

#include <DirectXMath.h> // 3D 행렬 및 벡터 수학 연산을 위한 DirectX 헤더를 포함합니다.
#include <vector> //   자식 트랜스폼들을 담을 가변 배열을 사용하기 위해 포함합니다.  

//   [28단계] 부모-자식 계층 구조(Hierarchy)를 지원하도록 업그레이드된 트랜스폼 클래스입니다.  
class Transform // Transform 클래스를 선언합니다.
{ // 클래스 내부 블록 시작입니다.
public: // 외부에서 자유롭게 접근하여 수정할 수 있도록 public으로 엽니다.
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f }; // 3D 공간 상의 X, Y, Z 위치 좌표를 저장하는 변수입니다.
    DirectX::XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f }; // 라디안 단위의 X, Y, Z 회전 각도(오일러 각)를 저장합니다.
    DirectX::XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f }; // 물체의 X, Y, Z 배율(크기)을 저장하며, 기본값은 1배입니다.


    //   [핵심 추가점] 내 부모가 누구인지 기억하는 포인터입니다. (부모가 없으면 nullptr)  
    Transform* Parent = nullptr;

    //   [핵심 추가점] 내 밑에 달린 자식 트랜스폼들의 목록입니다.  
    std::vector<Transform*> Children;

    //   부모를 설정하고, 부모의 자식 목록에 나를 등록하는 함수입니다.  
    void SetParent(Transform* newParent)
    { // 함수 시작
        Parent = newParent; // 내 부모 포인터를 갱신합니다.

        if (Parent != nullptr) // 만약 새 부모가 존재한다면
        { // 조건문 시작
            Parent->Children.push_back(this); // 부모의 자식 명부에 내 주소(this)를 추가합니다.
        } // 조건문 끝
    } // 함수 끝

     //   순수하게 '나 자신만의' 변환 행렬(Local Matrix)을 계산하는 함수입니다.  
    DirectX::XMMATRIX GetLocalMatrix() const
    { // 함수 시작
        DirectX::XMMATRIX S = DirectX::XMMatrixScaling(Scale.x, Scale.y, Scale.z); // 크기 행렬을 생성합니다.
        DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z); // 회전 행렬을 생성합니다.
        DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(Position.x, Position.y, Position.z); // 이동 행렬을 생성합니다.

        return S * R * T; // S * R * T 순서로 곱하여 로컬 행렬을 반환합니다.
    } // 함수 끝

    //   내 로컬 행렬에 부모의 행렬을 누적 곱셈하여 '최종 월드 공간 행렬'을 뽑아냅니다!  
    DirectX::XMMATRIX GetWorldMatrix() const
    { // 함수 시작
        DirectX::XMMATRIX local = GetLocalMatrix(); // 먼저 내 로컬 행렬을 구합니다.

        if (Parent != nullptr) // 만약 나에게 부모가 존재한다면!
        { // 조건문 시작
            // 내 로컬 행렬에 부모의 월드 행렬을 곱합니다. (재귀적으로 최상위 부모까지 거슬러 올라가 곱해집니다!)
            return local * Parent->GetWorldMatrix();
        } // 조건문 끝

        return local; // 부모가 없다면 내 로컬 행렬이 곧 월드 행렬입니다.
    } // 함수 끝
}; // 클래스 블록 끝입니다.