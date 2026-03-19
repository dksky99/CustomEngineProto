#pragma once // 헤더 중복 방지입니다.

#include <d3d12.h> // DX12 기능을 위해 포함합니다.
#include <wrl.h> // 스마트 포인터를 위해 포함합니다.
#include <DirectXMath.h> // 수학 연산을 위해 포함합니다.


#include <string> //  파일 경로 문자열을 다루기 위해 포함합니다. 
#include <vector> //  정점 동적 배열을 다루기 위해 포함합니다. 

//  렌더러에 있던 정점(Vertex) 데이터와 버퍼 생성 로직을 완전히 뜯어와 독립시켰습니다. 
struct Vertex // 정점 1개의 데이터를 담는 구조체입니다.
{ // 구조체 시작입니다.
    DirectX::XMFLOAT3 Pos; // 3D 위치입니다.
    DirectX::XMFLOAT4 Color; // 색상 데이터입니다.
    DirectX::XMFLOAT3 Normal; // 빛을 계산할 법선 벡터입니다.
    DirectX::XMFLOAT2 TexC; // 텍스처를 입힐 UV 좌표입니다.
}; // 구조체 끝입니다.

class Mesh // 3D 모델의 정점과 인덱스 데이터를 GPU에 보관하는 클래스입니다.
{ // 클래스 시작입니다.
public: // 퍼블릭 구역입니다.
    // 큐브(정육면체) 형태의 데이터를 GPU 메모리에 생성하는 초기화 함수입니다.
    void CreateBox(ID3D12Device* device);

    //  외부 .obj 파일의 경로를 받아 3D 모델을 로드하는 함수입니다. 
    bool LoadFromOBJ(const std::string& filepath, ID3D12Device* device);

    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return mVertexBufferView; } // 정점 버퍼 안경을 반환합니다.
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const { return mIndexBufferView; } // 인덱스 버퍼 안경을 반환합니다.
    UINT GetIndexCount() const { return mIndexCount; } // 그려야 할 총 인덱스 개수를 반환합니다.

private:
    //  CreateBox와 LoadFromOBJ에서 중복되는 'GPU 버퍼 생성' 코드를 하나로 묶은 헬퍼 함수입니다. 
    void CreateBuffers(ID3D12Device* device, const std::vector<Vertex>& vertices, const std::vector<std::uint16_t>& indices);


private: // 프라이빗 구역입니다.
    Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBuffer; // 정점 데이터가 담길 실제 GPU 메모리 리소스입니다.
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView; // 정점 버퍼를 읽기 위한 뷰(안경)입니다.

    Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBuffer; // 인덱스 데이터가 담길 GPU 메모리 리소스입니다.
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView; // 인덱스 버퍼를 읽기 위한 뷰(안경)입니다.
    UINT mIndexCount = 0; // 총 인덱스 개수입니다.
}; // 클래스 끝입니다.