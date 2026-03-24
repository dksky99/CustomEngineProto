#include "CameraComponent.h" // 자신의 헤더 파일을 포함합니다.
#include "../Core/InputManager.h" // 새로 만든 입력 관리자에게 키보드/마우스 상태를 물어보기 위해 포함합니다. 
#include "../Core/Actor.h" // 내가 장착된 주인의 위치(Transform)를 직접 조종하기 위해 포함합니다.
#include <algorithm> // 상하 시야 각도를 제한(clamp)하기 위해 포함합니다.

using namespace DirectX; // DX 수학 연산을 편하게 쓰기 위해 네임스페이스를 개방합니다.

CameraComponent::CameraComponent() // 생성자 구현부입니다.
{ // 함수 시작
    XMStoreFloat4x4(&mView, XMMatrixIdentity()); // 쓰레기값이 안 들어가게 뷰 행렬을 단위행렬로 초기화합니다.
    XMStoreFloat4x4(&mProj, XMMatrixIdentity()); // 투영 행렬도 단위행렬로 싹 비웁니다.
} // 함수 끝

void CameraComponent::SetLens(float fovY, float aspect, float zn, float zf) // 렌즈 장착 함수입니다.
{ // 함수 시작
    XMMATRIX P = XMMatrixPerspectiveFovLH(fovY, aspect, zn, zf); // 넘어온 스펙으로 원근 투영(Perspective) 행렬을 찍어냅니다.
    XMStoreFloat4x4(&mProj, P); // 찍어낸 행렬을 내 멤버 변수에 예쁘게 포장해 보관합니다.
} // 함수 끝

void CameraComponent::Update(float deltaTime) // 매 프레임 부품이 알아서 작동하는 업데이트 함수입니다.
{ // 함수 시작
    InputManager& input = InputManager::GetInstance(); // 중앙 통제실(입력 관리자)에 접속합니다.

    //  [핵심 로직] 이 부품이 장착된 주인 액터의 Transform(위치/회전) 포인터를 가져옵니다. 
    Transform* transform = mOwner->GetTransform();

    if (input.IsMouseRButtonDown()) // 우클릭을 누르고 있을 때만 시야를 회전시킵니다.
    { // 회전 블록 시작
        POINT delta = input.GetMouseDelta(); // 이번 프레임 마우스가 얼만큼 휙 움직였는지 픽셀 이동량을 받아옵니다.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(delta.x)); // X축 이동량을 라디안 각도로 변환합니다.
        float dy = XMConvertToRadians(0.25f * static_cast<float>(delta.y)); // Y축 이동량을 라디안 각도로 변환합니다.

        // 내 변수가 아니라, 주인(Actor)의 회전(Rotation) 데이터를 직접 덮어씁니다! 
        transform->Rotation.y += dx; // 마우스 좌우 이동은 목을 도리도리(Yaw) 하는 것에 누적합니다.
        transform->Rotation.x += dy; // 마우스 상하 이동은 고개를 끄덕(Pitch) 하는 것에 누적합니다.
        transform->Rotation.x = std::clamp(transform->Rotation.x, -1.5f, 1.5f); // 고개가 등 뒤로 꺾이지 않게 위아래 각도를 제한합니다.
    } // 회전 블록 끝

    // 주인의 최신 회전(Pitch, Yaw) 값을 바탕으로 3D 공간의 회전 행렬을 생성합니다.
    XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(transform->Rotation.x, transform->Rotation.y, 0.0f);

    // 회전 행렬을 이용해 현재 카메라가 바라보는 정면(Forward), 우측(Right), 위쪽(Up) 방향 화살표(벡터)를 뽑아냅니다.
    XMVECTOR forward = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotMat);
    XMVECTOR right = XMVector3TransformNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotMat);
    XMVECTOR up = XMVector3TransformNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotMat);

    XMVECTOR pos = XMLoadFloat3(&transform->Position); // 현재 주인의 3D 공간 위치를 수학 벡터로 불러옵니다.
    float moveSpeed = 10.0f * deltaTime; // 1초에 10유닛을 이동하도록 스피드를 세팅합니다.

    // 입력 관리자에게 키보드가 눌렸는지 물어보고, 눌렸다면 주인의 위치를 해당 방향 화살표 쪽으로 밀어버립니다!
    if (input.IsKeyPress('W')) pos += forward * moveSpeed; // 앞으로 전진
    if (input.IsKeyPress('S')) pos -= forward * moveSpeed; // 뒤로 후진
    if (input.IsKeyPress('D')) pos += right * moveSpeed; // 우측 게걸음
    if (input.IsKeyPress('A')) pos -= right * moveSpeed; // 좌측 게걸음
    if (input.IsKeyPress('E')) pos += up * moveSpeed; // 드론처럼 수직 상승
    if (input.IsKeyPress('Q')) pos -= up * moveSpeed; // 수직 하강

    XMStoreFloat3(&transform->Position, pos); // 새로 계산된 이동 위치를 주인(Actor)의 몸통에 완전히 덮어씌웁니다!

    XMVECTOR target = pos + forward; // 현재 위치에서 정면 화살표를 더해 "내가 바라볼 1m 앞의 과녁"을 세팅합니다.
    XMMATRIX view = XMMatrixLookAtLH(pos, target, up); // 내 위치, 과녁, 윗방향을 이용해 "세상을 내 시야로 끌고 오는" 뷰 행렬을 만듭니다.
    XMStoreFloat4x4(&mView, view); // 완성된 뷰 행렬을 렌더러가 쓸 수 있게 저장합니다.
} // 함수 끝

DirectX::XMMATRIX CameraComponent::GetView() const { return XMLoadFloat4x4(&mView); } // 뷰 행렬 반환
DirectX::XMMATRIX CameraComponent::GetProj() const { return XMLoadFloat4x4(&mProj); } // 투영 행렬 반환

// 광택 처리를 위해 카메라의 위치를 반환할 때도, 자기 변수가 아니라 자기가 부착된 주인 액터의 위치를 보고합니다!
DirectX::XMFLOAT3 CameraComponent::GetPosition() const { return mOwner->GetTransform()->Position; }