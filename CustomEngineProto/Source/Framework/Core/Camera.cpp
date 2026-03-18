#include "Camera.h" // 자신의 헤더 파일을 포함합니다.
#include <algorithm> // 상하 각도 제한(std::clamp)을 사용하기 위해 포함합니다.

using namespace DirectX; // DirectX 수학 라이브러리 네임스페이스를 사용합니다.

Camera::Camera() // 생성자 구현부입니다.
{ // 함수 블록 시작입니다.
    // 뷰 행렬과 투영 행렬을 쓰레기값이 없도록 기본 단위 행렬(Identity)로 꽉 채워 초기화합니다.
    XMStoreFloat4x4(&mView, XMMatrixIdentity());
    XMStoreFloat4x4(&mProj, XMMatrixIdentity());
} // 함수 블록 끝입니다.

Camera::~Camera() // 소멸자 구현부입니다.
{ // 함수 블록 시작입니다.
} // 함수 블록 끝입니다.

void Camera::SetLens(float fovY, float aspect, float zn, float zf) // 렌즈 세팅 함수 구현부입니다.
{ // 함수 블록 시작입니다.
    // 파라미터로 받은 시야각, 화면 비율, 깊이 정보를 바탕으로 원근감(Perspective)을 나타내는 투영 행렬을 조립합니다.
    XMMATRIX P = XMMatrixPerspectiveFovLH(fovY, aspect, zn, zf);
    // 조립된 행렬을 멤버 변수에 저장해 둡니다.
    XMStoreFloat4x4(&mProj, P);
} // 함수 블록 끝입니다.

//  [구조 분화] 기존에 렌더러의 Update 함수에 길고 지저분하게 섞여 있던 WASD 및 마우스 처리 로직이 이곳으로 예쁘게 독립했습니다! 
void Camera::Update(float deltaTime) // 카메라 업데이트 구현부입니다.
{ // 함수 블록 시작입니다.
    POINT currentMousePos; // 현재 마우스 커서의 픽셀 위치를 담을 변수입니다.
    GetCursorPos(&currentMousePos); // 윈도우 API를 이용해 커서 위치를 얻어옵니다.

    // 마우스 우클릭 버튼이 눌려있을 때만 카메라 시야를 회전시킵니다.
    if ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0)
    { // 우클릭 블록 시작
        // 마우스의 이전 위치와 현재 위치의 픽셀 차이(Delta)를 구하고, 이를 라디안(각도)으로 부드럽게 변환합니다.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(currentMousePos.x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(currentMousePos.y - mLastMousePos.y));

        mYaw += dx; // 마우스 좌우 이동량을 좌우 회전각에 누적합니다.
        mPitch += dy; // 마우스 상하 이동량을 상하 회전각에 누적합니다.
        mPitch = std::clamp(mPitch, -1.5f, 1.5f); // 목이 등 뒤로 꺾이지 않도록 위아래 시야를 물리적으로 제한합니다.
    } // 우클릭 블록 끝

    mLastMousePos = currentMousePos; // 다음 프레임 연산을 위해 현재 위치를 과거 위치로 기억해 둡니다.

    // 카메라의 누적된 회전각을 바탕으로 카메라 자기 자신만의 3D 회전 행렬을 생성합니다.
    XMMATRIX camRotation = XMMatrixRotationRollPitchYaw(mPitch, mYaw, 0.0f);

    // 회전 행렬을 이용해 카메라가 바라보는 정면(Forward), 우측(Right), 윗면(Up)의 방향 화살표(벡터)를 추출합니다.
    XMVECTOR camForward = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), camRotation);
    XMVECTOR camRight = XMVector3TransformNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), camRotation);
    XMVECTOR camUp = XMVector3TransformNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), camRotation);

    XMVECTOR camPos = XMLoadFloat3(&mPosition); // 계산을 위해 XMFLOAT3 위치를 XMVECTOR로 변환합니다.
    float moveSpeed = 5.0f * deltaTime; // 1초에 5 유닛을 이동하도록 속도를 설정합니다.

    // 키보드 입력을 받아 해당 방향 벡터로 카메라 위치를 더하거나 뺍니다.
    if (GetAsyncKeyState('W') & 0x8000) camPos += camForward * moveSpeed; // 앞으로 전진
    if (GetAsyncKeyState('S') & 0x8000) camPos -= camForward * moveSpeed; // 뒤로 후진
    if (GetAsyncKeyState('D') & 0x8000) camPos += camRight * moveSpeed;   // 우측 게걸음
    if (GetAsyncKeyState('A') & 0x8000) camPos -= camRight * moveSpeed;   // 좌측 게걸음
    if (GetAsyncKeyState('E') & 0x8000) camPos += camUp * moveSpeed;      // 수직 상승 (드론 샷)
    if (GetAsyncKeyState('Q') & 0x8000) camPos -= camUp * moveSpeed;      // 수직 하강

    XMStoreFloat3(&mPosition, camPos); // 업데이트된 위치 벡터를 다시 멤버 변수에 저장합니다.

    XMVECTOR camTarget = camPos + camForward; // 카메라 위치에 정면 방향 벡터를 더하여, 카메라가 쳐다볼 '과녁(Target)' 점을 잡습니다.

    // 카메라 위치, 타겟 위치, 위쪽 방향을 넣어 '세상을 카메라 시점으로 끌고 오는' 최종 View 행렬을 만듭니다.
    XMMATRIX view = XMMatrixLookAtLH(camPos, camTarget, camUp);
    XMStoreFloat4x4(&mView, view); // 완성된 View 행렬을 저장합니다.
} // 함수 블록 끝입니다.

DirectX::XMMATRIX Camera::GetView() const // 뷰 행렬 반환 함수입니다.
{ // 함수 블록 시작
    return XMLoadFloat4x4(&mView); // 연산용 행렬 타입으로 변환하여 반환합니다.
} // 함수 블록 끝

DirectX::XMMATRIX Camera::GetProj() const // 투영 행렬 반환 함수입니다.
{ // 함수 블록 시작
    return XMLoadFloat4x4(&mProj); // 연산용 행렬 타입으로 변환하여 반환합니다.
} // 함수 블록 끝