#pragma once // 헤더 파일이 중복으로 포함되는 것을 방지합니다.

#include <d3d12.h> // DX12의 핵심 기능을 사용하기 위해 포함합니다.
#include <wrl.h> // ComPtr(스마트 포인터)을 사용하기 위해 포함합니다.
#include <string> // 텍스처 파일의 이름이나 경로를 다루기 위해 포함합니다.

//  GPU 메모리에 올라가는 이미지 데이터를 전담하는 텍스처 리소스 클래스입니다. 
class Texture // Texture 클래스를 선언합니다.
{ // 클래스 블록 시작입니다.
public: // 누구나 접근할 수 있도록 퍼블릭으로 엽니다.
    // 기존 렌더러에 있던 체크무늬 생성 로직을 텍스처 클래스 내부로 캡슐화한 함수입니다.
    void CreateCheckerboard(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
    // 외부 이미지 파일(PNG, JPG 등)을 읽어오는 함수 선언을 추가합니다. 
  // 와이드 문자열(wstring)로 경로를 받으며, 디바이스와 명령서를 전달받아 GPU에 올립니다.
    bool LoadFromFile(const std::wstring& filepath, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

    // 실제 GPU 메모리에 할당된 텍스처 리소스의 포인터를 반환하는 함수입니다.
    Microsoft::WRL::ComPtr<ID3D12Resource> GetResource() const { return mTexture; }

private: // 외부에서 함부로 조작하지 못하게 은닉합니다.
    Microsoft::WRL::ComPtr<ID3D12Resource> mTexture; // 실제 이미지 데이터가 담길 GPU 전용 메모리 공간입니다.
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadHeap; // CPU에서 GPU로 데이터를 넘길 때 잠시 거쳐 가는 임시 업로드 메모리입니다.
}; // 클래스 블록 끝입니다.