#pragma once // 헤더 파일 중복 포함 방지입니다.
#include "Component.h" // 부모 컴포넌트 클래스를 포함합니다.
#include <DirectXMath.h> // 3D 수학 연산 데이터를 위해 포함합니다.

//  물체에 '빛(조명)' 속성을 부여하는 조명 부품입니다! (언리얼의 ULightComponent 역할) 
class LightComponent : public Component // Component 상속을 받습니다.
{ // 클래스 블록 시작입니다.
public: // 누구나 접근 가능합니다.
    DirectX::XMFLOAT4 LightColor = { 1.0f, 1.0f, 1.0f, 1.0f }; //  렌더러로 전달할 빛의 최종 색상입니다.
    float Intensity = 1.0f; //  빛의 밝기(강도) 수치입니다.
}; // 클래스 블록 끝입니다.