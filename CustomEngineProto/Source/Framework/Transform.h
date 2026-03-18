#pragma once // 헤더 파일이 중복으로 포함되는 것을 방지합니다.

#include <DirectXMath.h> // 3D 행렬 및 벡터 수학 연산을 위한 DirectX 헤더를 포함합니다.

//   3D 공간에서 물체의 위치, 회전, 크기를 정의하는 트랜스폼 클래스입니다. 
class Transform // Transform 클래스를 선언합니다.
{ // 클래스 내부 블록 시작입니다.
public: // 외부에서 자유롭게 접근하여 수정할 수 있도록 public으로 엽니다.
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f }; // 3D 공간 상의 X, Y, Z 위치 좌표를 저장하는 변수입니다.
    DirectX::XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f }; // 라디안 단위의 X, Y, Z 회전 각도(오일러 각)를 저장합니다.
    DirectX::XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f }; // 물체의 X, Y, Z 배율(크기)을 저장하며, 기본값은 1배입니다.

    // 현재의 위치, 회전, 크기 변수들을 조합하여 렌더러가 사용할 수 있는 '최종 월드 변환 행렬'을 만들어 반환하는 함수입니다.
    DirectX::XMMATRIX GetWorldMatrix() const // 멤버 변수를 수정하지 않으므로 const 함수로 선언합니다.
    { // 함수 블록 시작입니다.
        // 1. 크기(Scale) 데이터를 이용해 크기 변환 행렬을 생성합니다.
        DirectX::XMMATRIX S = DirectX::XMMatrixScaling(Scale.x, Scale.y, Scale.z);
        // 2. 회전(Rotation) 데이터를 이용해 회전 변환 행렬을 생성합니다. (Roll-Pitch-Yaw 순서를 따릅니다)
        DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z);
        // 3. 위치(Position) 데이터를 이용해 이동(Translation) 변환 행렬을 생성합니다.
        DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(Position.x, Position.y, Position.z);

        // 4. 크기 -> 회전 -> 이동 (SRT) 순서로 행렬을 곱하여 물체의 최종 공간 상태를 결정짓고 반환합니다.
        return S * R * T;
    } // 함수 블록 끝입니다.
}; // 클래스 블록 끝입니다.