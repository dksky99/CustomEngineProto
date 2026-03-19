#pragma once // 헤더 파일 중복 포함 방지입니다.

#include "Component.h" // 부모 클래스인 Component 헤더를 포함합니다.
#include <memory> // 스마트 포인터를 사용하기 위해 포함합니다.

class Mesh; // 장착할 외형 데이터인 Mesh 클래스를 전방 선언합니다.
class Material; //   머티리얼 클래스를 전방 선언하여 부품으로 쓸 수 있게 합니다.  

//  액터에게 3D 외형(껍데기)을 부여하는 메시 컴포넌트입니다. (언리얼의 UStaticMeshComponent 역할) 
class MeshComponent : public Component // Component를 상속받습니다.
{ // 클래스 블록 시작입니다.
public: // 퍼블릭 구역입니다.
    void SetMesh(std::shared_ptr<Mesh> mesh) { mMesh = mesh; } // 이 부품에 3D 모델(메시) 데이터를 끼워 넣습니다.
    std::shared_ptr<Mesh> GetMesh() const { return mMesh; } // 현재 장착된 3D 모델 데이터를 반환합니다.
    //   [추가점] 3D 모델 껍데기뿐만 아니라, 표면 재질(머티리얼)도 세팅할 수 있는 함수를 뚫어줍니다!  
    void SetMaterial(std::shared_ptr<Material> mat) { mMaterial = mat; } // 머티리얼을 장착합니다.
    std::shared_ptr<Material> GetMaterial() const { return mMaterial; } // 장착된 머티리얼을 반환합니다.
private: // 프라이빗 구역입니다.
    std::shared_ptr<Mesh> mMesh; // 실제 정점 데이터를 들고 있는 메시 객체의 스마트 포인터입니다.
    std::shared_ptr<Material> mMaterial; //   재질 데이터 포인터가 추가되었습니다!  
}; // 클래스 블록 끝입니다.