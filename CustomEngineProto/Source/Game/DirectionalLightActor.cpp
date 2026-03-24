#include "DirectionalLightActor.h" // 자신의 헤더를 포함합니다.
#include "Framework/Components/LightComponent.h" // 조명 부품 헤더를 포함합니다.
#include <algorithm> // clamp 함수를 쓰기 위해 포함합니다.

using namespace DirectX; // DX 수학 네임스페이스를 엽니다.

DirectionalLightActor::DirectionalLightActor() // 생성자 구현부입니다.
{ // 함수 시작
    mLightComp = std::make_shared<LightComponent>(); //  조명 부품을 메모리에 하나 찍어냅니다.
    AddComponent(mLightComp); //  내 몸통(Actor)에 조명 부품을 조립해 넣습니다!

    // 태양이 완전히 수직으로만 돌지 않고 살짝 비스듬하게 돌도록 Y축(Yaw) 각도를 미리 0.5 라디안 정도 틀어줍니다.
    mTransform.Rotation.y = 0.5f;
} // 함수 끝

//  [구조 분화] 기존에 렌더러에 흉측하게 하드코딩되어 있던 낮과 밤 계산 로직이 이 액터 안으로 우아하게 분리되었습니다! 
void DirectionalLightActor::Update(float deltaTime) // 매 프레임 업데이트입니다.
{ // 함수 시작
    Actor::Update(deltaTime); //  부모의 업데이트를 실행해 컴포넌트들도 작동하게 합니다.

    //  X축(Pitch)을 시간의 흐름에 따라 계속 회전시켜 태양이 뜨고 지는 일주 운동을 시뮬레이션합니다. 
    mTransform.Rotation.x += deltaTime * 0.5f;

    // 현재 내 회전 각도를 바탕으로 3D 회전 행렬을 뽑아냅니다.
    XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(mTransform.Rotation.x, mTransform.Rotation.y, mTransform.Rotation.z);

    // 회전 행렬을 이용해 내 정면(Forward)이 어디를 가리키는지 방향 벡터를 추출합니다. (이것이 렌더러가 쓸 '빛의 방향'입니다!)
    XMVECTOR lightDir = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotMat);

    XMFLOAT3 dir; // 계산용 구조체
    XMStoreFloat3(&dir, lightDir); // 추출한 방향 벡터를 일반 실수형으로 변환합니다.

    // 빛의 방향이 아래를 향할 때(-Y) 지상에서는 낮이 되므로, 역방향을 고도로 삼아 계산합니다.
    float sunY = -dir.y;

    // 태양의 고도에 따른 빛의 강도(0.0 ~ 1.0)를 계산합니다.
    float intensity = std::clamp(sunY + 0.2f, 0.0f, 1.0f);

    // 내 몸에 달린 조명 부품(LightComponent)의 색상과 밝기 데이터를 매 프레임 최신화합니다! 
    // 이렇게 세팅만 해두면 렌더러가 나중에 알아서 이 부품을 찾아가 데이터를 빼갑니다.
    mLightComp->LightColor = { intensity, intensity * 0.9f, intensity * 0.8f, 1.0f };
    mLightComp->Intensity = intensity;
} // 함수 끝