#pragma once // 헤더 파일 중복 포함 방지입니다.
#include "Component.h" // 부모 컴포넌트 클래스를 포함합니다.
#include <DirectXMath.h> // 3D 수학 연산 데이터를 위해 포함합니다.

//  조명의 종류를 선택할 수 있는 열거형(Enum)을 추가합니다! 
enum class ELightType
{
    Directional, // 태양빛처럼 거리에 상관없이 한 방향으로 평행하게 쏘는 빛
    Point        // 전구처럼 한 점에서 사방으로 뻗어 나가며, 거리가 멀어지면 약해지는 빛
};

//  물체에 '빛(조명)' 속성을 부여하는 조명 부품입니다! (언리얼의 ULightComponent 역할) 
class LightComponent : public Component // Component 상속을 받습니다.
{ // 클래스 블록 시작입니다.
public: // 누구나 접근 가능합니다.
    //  이 부품의 기본 타입을 직사광선으로 둡니다. 
    ELightType Type = ELightType::Directional;

    DirectX::XMFLOAT4 LightColor = { 1.0f, 1.0f, 1.0f, 1.0f }; //  렌더러로 전달할 빛의 최종 색상입니다.
    float Intensity = 1.0f; //  빛의 밝기(강도) 수치입니다.

    //  점광원(Point Light) 전용 '거리 감쇠(Attenuation)' 속성입니다! 
    float FalloffStart = 1.0f; // 이 거리까지는 빛이 100% 강도로 유지됩니다.
    float FalloffEnd = 10.0f;  // 이 거리를 넘어가면 빛이 0%로 완전히 소멸합니다.

}; // 클래스 블록 끝입니다.