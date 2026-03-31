#pragma once // 헤더 파일 중복 방지를 위한 지시어입니다.

#include <memory> // 스마트 포인터를 사용하기 위해 포함합니다.
#include <DirectXMath.h> // 색상과 반사율 데이터를 담기 위해 수학 라이브러리를 포함합니다.

class Texture; // 머티리얼이 텍스처를 들고 있을 수 있도록 전방 선언을 합니다.

//  물체의 표면 재질(빛 반사, 색상, 텍스처)을 정의하는 상용 엔진의 핵심 클래스입니다. 
class Material // Material 클래스 선언입니다.
{ // 클래스 블록 시작입니다.
public: // 누구나 접근 가능합니다.
    // 이 재질에 입혀질 기본 이미지(텍스처)의 포인터입니다. (언리얼의 Base Color 텍스처 역할)
    std::shared_ptr<Texture> DiffuseMap = nullptr;

    // 머티리얼이 이제 빛과 그림자를 왜곡시킬 '노멀맵'도 가질 수 있습니다! 
    std::shared_ptr<Texture> NormalMap = nullptr;

    // 텍스처가 없을 때 기본적으로 칠해질 RGBA 색상값입니다. 초기값은 순백색(1,1,1,1)입니다.
    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };

    // 어두운 곳에서도 스스로 빛을 내는 '발광(Emissive)' 색상 변수를 추가합니다!
   // 이 수치를 높게 주면 우리가 만든 포스트 프로세싱(Bloom) 필터가 반응하여 픽셀을 눈부시게 번지게 만듭니다! 
    DirectX::XMFLOAT3 Emissive = { 0.0f, 0.0f, 0.0f };

    //  PBR 물리 기반 렌더링을 위한 핵심 변수 2가지입니다! 
    float Roughness = 0.5f; // 거칠기 (0.0 = 완벽한 거울, 1.0 = 완벽한 무광 찰흙)
    float Metallic = 0.0f;  // 금속성 (0.0 = 비금속/돌, 1.0 = 순수 금속)

}; // 클래스 블록 끝입니다.