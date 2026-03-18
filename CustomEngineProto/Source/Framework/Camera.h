#pragma once // 헤더 중복 포함을 방지합니다.

#include <windows.h> // POINT 구조체 및 키보드/마우스 입력(GetAsyncKeyState)을 위해 포함합니다.
#include <DirectXMath.h> // 3D 수학 연산(행렬, 벡터)을 위해 포함합니다.

//  [신규 파일] 렌더러에 묶여있던 시점(View)과 원근감(Projection) 로직을 분리한 독립적인 카메라 클래스입니다. 
class Camera // 카메라 클래스를 선언합니다.
{ // 클래스 블록 시작입니다.
public: // 누구나 접근 가능하게 엽니다.
    Camera(); // 카메라 생성자입니다.
    ~Camera(); // 카메라 소멸자입니다.

    // 렌즈의 스펙(투영 행렬)을 설정하는 함수입니다. (시야각, 화면 비율, 최소 깊이, 최대 깊이)
    void SetLens(float fovY, float aspect, float zn, float zf);

    // 매 프레임 사용자의 키보드와 마우스 입력을 받아 카메라의 위치와 회전(시선)을 갱신합니다.
    void Update(float deltaTime);

    // 카메라의 위치와 시선을 바탕으로 완성된 '뷰 행렬(View Matrix)'을 반환합니다.
    DirectX::XMMATRIX GetView() const;

    // 카메라의 렌즈 설정을 바탕으로 완성된 '투영 행렬(Projection Matrix)'을 반환합니다.
    DirectX::XMMATRIX GetProj() const;

    // 물체의 광택(Specular) 연산 등을 위해 카메라의 현재 3D 위치를 반환합니다.
    DirectX::XMFLOAT3 GetPosition() const { return mPosition; }

private: // 외부에서 함부로 조작하지 못하게 은닉합니다.
    DirectX::XMFLOAT3 mPosition = { 0.0f, 15.0f, -15.0f }; // 월드 공간 상의 초기 카메라 위치입니다.
    float mPitch = 0.7f; // 카메라의 상하 회전각(고개 숙임/들림)입니다.
    float mYaw = 0.0f; // 카메라의 좌우 회전각(도리도리)입니다.

    DirectX::XMFLOAT4X4 mView; // 뷰 행렬을 저장할 멤버 변수입니다.
    DirectX::XMFLOAT4X4 mProj; // 투영 행렬을 저장할 멤버 변수입니다.

    POINT mLastMousePos = { 0, 0 }; // 이전 프레임의 마우스 좌표를 기억하여 이동량(Delta)을 구하기 위한 변수입니다.
}; // 클래스 블록 끝입니다.