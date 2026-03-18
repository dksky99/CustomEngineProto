#include "Mesh.h" // 자신의 헤더 파일을 포함합니다.
#include "d3dx12.h" // 메모리 할당을 돕는 DX12 헬퍼 헤더를 포함합니다.
#include <vector> // 정점 배열을 만들기 위해 포함합니다.

// 렌더러에 있던 흉측한 하드코딩 배열들을 Mesh 클래스로 몰아넣어 렌더러를 순수하게 만듭니다.
void Mesh::CreateBox(ID3D12Device* device) // 큐브 데이터를 생성하고 GPU에 올리는 함수입니다.
{ // 함수 시작입니다.
    std::vector<Vertex> vertices = // 24개의 고유 정점 데이터를 배열로 선언합니다.
    { // 배열 데이터 시작입니다.
        { {-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f} }, // 앞면 좌하
        { {-0.5f, +0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} }, // 앞면 좌상
        { {+0.5f, +0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f} }, // 앞면 우상
        { {+0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} }, // 앞면 우하

        { {-0.5f, -0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} }, // 뒷면 좌하
        { {+0.5f, -0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} }, // 뒷면 우하
        { {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} }, // 뒷면 우상
        { {-0.5f, +0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} }, // 뒷면 좌상

        { {-0.5f, +0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} }, // 윗면 좌하
        { {-0.5f, +0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} }, // 윗면 좌상
        { {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} }, // 윗면 우상
        { {+0.5f, +0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} }, // 윗면 우하

        { {-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} }, // 아랫면 좌상
        { {+0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} }, // 아랫면 우상
        { {+0.5f, -0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} }, // 아랫면 우하
        { {-0.5f, -0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} }, // 아랫면 좌하

        { {-0.5f, -0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} }, // 좌측면 우하
        { {-0.5f, +0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} }, // 좌측면 우상
        { {-0.5f, +0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} }, // 좌측면 좌상
        { {-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} }, // 좌측면 좌하

        { {+0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} }, // 우측면 좌하
        { {+0.5f, +0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} }, // 우측면 좌상
        { {+0.5f, +0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} }, // 우측면 우상
        { {+0.5f, -0.5f, +0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} }  // 우측면 우하
    }; // 배열 데이터 끝입니다.

    std::vector<std::uint16_t> indices = // 위 정점들을 묶을 36개의 인덱스 배열입니다.
    { // 인덱스 시작입니다.
        0, 1, 2,  0, 2, 3,       // 앞면
        4, 5, 6,  4, 6, 7,       // 뒷면
        8, 9, 10, 8, 10, 11,     // 윗면
        12, 13, 14, 12, 14, 15,  // 아랫면
        16, 17, 18, 16, 18, 19,  // 좌측면
        20, 21, 22, 20, 22, 23   // 우측면
    }; // 인덱스 끝입니다.

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex); // 정점 배열의 총 바이트 크기입니다.
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t); // 인덱스 배열의 총 바이트 크기입니다.
    mIndexCount = (UINT)indices.size(); // 총 인덱스 개수를 저장합니다.

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD); // CPU에서 바로 덮어쓸 수 있는 업로드 힙을 설정합니다.
    CD3DX12_RESOURCE_DESC vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize); // 정점 버퍼 스펙을 정의합니다.

    // 정점 버퍼 메모리를 GPU에 할당합니다.
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vbDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mVertexBuffer));

    UINT8* pVertexDataBegin; // 메모리 맵핑 포인터입니다.
    mVertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pVertexDataBegin)); // GPU 메모리 주소를 CPU로 가져옵니다.
    memcpy(pVertexDataBegin, vertices.data(), vbByteSize); // C++ 배열 데이터를 GPU 메모리로 복사합니다.
    mVertexBuffer->Unmap(0, nullptr); // 포인터 연결을 해제합니다.

    mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress(); // 뷰에 실제 GPU 주소를 매핑합니다.
    mVertexBufferView.StrideInBytes = sizeof(Vertex); // 정점 1개의 크기를 알려줍니다.
    mVertexBufferView.SizeInBytes = vbByteSize; // 전체 크기를 알려줍니다.

    CD3DX12_RESOURCE_DESC ibDesc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize); // 인덱스 버퍼 스펙을 정의합니다.

    // 인덱스 버퍼 메모리를 GPU에 할당합니다.
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &ibDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mIndexBuffer));

    UINT8* pIndexDataBegin; // 메모리 맵핑 포인터입니다.
    mIndexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pIndexDataBegin)); // GPU 메모리 주소를 가져옵니다.
    memcpy(pIndexDataBegin, indices.data(), ibByteSize); // 인덱스 배열을 복사합니다.
    mIndexBuffer->Unmap(0, nullptr); // 연결 해제합니다.

    mIndexBufferView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress(); // 뷰에 주소를 매핑합니다.
    mIndexBufferView.Format = DXGI_FORMAT_R16_UINT; // 16비트 부호 없는 정수 포맷임을 알려줍니다.
    mIndexBufferView.SizeInBytes = ibByteSize; // 전체 크기를 알려줍니다.
} // 함수 끝입니다.