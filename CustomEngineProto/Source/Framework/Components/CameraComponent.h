#pragma once // 헤더 파일 중복 포함 방지입니다.

#include "Component.h" // 컴포넌트 시스템에 편입되기 위해 부모 클래스를 포함합니다.
#include <DirectXMath.h> // 3D 행렬 및 벡터 수학 연산을 위해 포함합니다.

//  허공을 떠돌던 카메라가 드디어 액터에 장착할 수 있는 객체 지향 '부품'으로 개조되었습니다! 
class CameraComponent : public Component // Component 클래스를 상속받습니다.
{ // 클래스 시작
public: // 퍼블릭
    CameraComponent(); // 카메라 부품 생성자입니다.

    // 렌즈의 속성(시야각, 화면비율, 최소깊이, 최대깊이)을 설정하여 투영 행렬을 만드는 함수입니다.
    void SetLens(float fovY, float aspect, float zn, float zf);

    // 부모(Component)의 가상 함수를 오버라이딩하여, 매 프레임 입력에 따라 주인의 위치를 갱신합니다.
    virtual void Update(float deltaTime) override;

    DirectX::XMMATRIX GetView() const; // 카메라 부품이 계산해 낸 뷰 행렬을 반환합니다.
    DirectX::XMMATRIX GetProj() const; // 카메라 부품이 계산해 낸 원근 투영 행렬을 반환합니다.

    //  자기 자신의 변수가 아니라, 자신이 장착된 주인(Actor)의 위치를 반환합니다! 
    DirectX::XMFLOAT3 GetPosition() const;

private: // 프라이빗
    DirectX::XMFLOAT4X4 mView; // 뷰 행렬을 저장할 메모리 공간입니다.
    DirectX::XMFLOAT4X4 mProj; // 투영 행렬을 저장할 메모리 공간입니다.
}; // 클래스 끝