#include "Mesh.h" // 자신의 헤더 파일을 포함합니다.
#include "d3dx12.h" // 메모리 할당을 돕는 DX12 헬퍼 헤더를 포함합니다.
#include <vector> // 정점 배열을 만들기 위해 포함합니다.

//  텍스트 파일(obj)을 읽고 파싱하기 위한 C++ 표준 라이브러리들을 포함합니다. 
#include <fstream> // 파일 입출력 스트림입니다.
#include <sstream> // 문자열 스트림 파싱을 위함입니다.
#include <iostream> // 디버그 출력을 위함입니다.


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

    CreateBuffers(device, vertices, indices);
} // 함수 끝입니다.

//  외부 OBJ 파일을 텍스트 기반으로 해석(Parsing)하는 전용 로더입니다. 
bool Mesh::LoadFromOBJ(const std::string& filepath, ID3D12Device* device)
{ // 함수 시작입니다.
    std::ifstream file(filepath); // 파일 경로를 이용해 읽기 모드로 파일을 엽니다.
    if (!file.is_open()) // 파일이 존재하지 않거나 열리지 않는다면
    { // 조건문 시작
        return false; // 실패를 반환하여 호출자에게 알립니다.
    } // 조건문 끝

    // OBJ 파일은 위치(v), UV(vt), 법선(vn)이 따로따로 저장되므로 임시로 담아둘 배열들을 만듭니다.
    std::vector<DirectX::XMFLOAT3> tempPositions; // 임시 위치 배열
    std::vector<DirectX::XMFLOAT2> tempTexcoords; // 임시 UV 배열
    std::vector<DirectX::XMFLOAT3> tempNormals;   // 임시 법선 배열

    std::vector<Vertex> vertices; // 최종적으로 조립될 DX12용 정점 배열입니다.
    std::vector<std::uint16_t> indices; // 최종 조립될 인덱스 배열입니다.

    std::string line; // 텍스트를 한 줄씩 읽어 담을 문자열 변수입니다.
    while (std::getline(file, line)) // 파일의 끝에 도달할 때까지 한 줄씩 계속 읽어옵니다.
    { // 루프 시작
        std::istringstream ss(line); // 한 줄의 문자열을 공백 단위로 쪼개기(Tokenize) 위한 스트림입니다.
        std::string type; // 각 줄의 첫 번째 단어(명령어 타입)를 담을 변수입니다.
        ss >> type; // 첫 번째 단어를 추출합니다. (예: "v", "vt", "f")

        if (type == "v") // 만약 정점 위치 데이터라면
        { // 조건문 시작
            float x, y, z;
            ss >> x >> y >> z; // 공백을 기준으로 x, y, z 숫자를 뽑아냅니다.
            tempPositions.push_back({ x, y, z }); // 임시 배열에 저장합니다.
        } // 조건문 끝
        else if (type == "vt") // 만약 텍스처 UV 좌표 데이터라면
        { // 조건문 시작
            float u, v;
            ss >> u >> v; // u, v 숫자를 뽑아냅니다.
            // OBJ 모델은 그래픽 툴(블렌더 등)과 DX12 간의 V축(세로축) 방향이 반대인 경우가 많아 1.0에서 빼줍니다.
            tempTexcoords.push_back({ u, 1.0f - v });
        } // 조건문 끝
        else if (type == "vn") // 만약 법선(Normal) 데이터라면
        { // 조건문 시작
            float x, y, z;
            ss >> x >> y >> z; // x, y, z 숫자를 뽑아냅니다.
            tempNormals.push_back({ x, y, z }); // 임시 배열에 저장합니다.
        } // 조건문 끝
        else if (type == "f") // 가장 중요한 면(Face) 조립 데이터라면 (예: "f 1/1/1 2/2/2 3/3/3")
        { // 조건문 시작
            // 삼각형이므로 3개의 점에 대해 조립을 진행합니다.
            for (int i = 0; i < 3; ++i)
            { // 내부 루프 시작
                std::string vertexData;
                ss >> vertexData; // "1/1/1" 같은 덩어리 하나를 뽑아냅니다.

                std::istringstream vss(vertexData); // 덩어리를 슬래시('/')로 쪼개기 위한 스트림입니다.
                std::string v, vt, vn;

                std::getline(vss, v, '/'); // 첫 슬래시 전까지의 문자열(위치 번호)을 가져옵니다.
                std::getline(vss, vt, '/'); // 두 번째 슬래시 전까지의 문자열(UV 번호)을 가져옵니다.
                std::getline(vss, vn, '/'); // 마지막 남은 문자열(법선 번호)을 가져옵니다.

                Vertex vertex = {}; // 완전히 깨끗한 새 정점을 하나 조립합니다.

                // OBJ 파일의 번호는 1번부터 시작하므로, C++ 배열(0번 시작)에 맞추기 위해 -1을 해줍니다.
                if (!v.empty()) vertex.Pos = tempPositions[std::stoi(v) - 1]; // 문자열을 숫자로 바꿔 위치 복사
                if (!vt.empty()) vertex.TexC = tempTexcoords[std::stoi(vt) - 1]; // UV 복사
                if (!vn.empty()) vertex.Normal = tempNormals[std::stoi(vn) - 1]; // 법선 복사
                vertex.Color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 색상은 무조건 흰색 기본값으로 줍니다.

                vertices.push_back(vertex); // 완성된 정점을 최종 배열에 밀어 넣습니다.
                indices.push_back((std::uint16_t)(vertices.size() - 1)); // 고유한 인덱스 번호를 순서대로 부여합니다.
            } // 내부 루프 끝
        } // 조건문 끝
    } // 루프 끝

    // 파싱이 완료되어 조립된 정점과 인덱스 배열을 GPU에 올리기 위해 헬퍼 함수를 호출합니다.
    CreateBuffers(device, vertices, indices);

    return true; // 성공적으로 로드되었음을 알립니다.
} // 함수 끝

// C++ 배열 데이터를 실제 DX12 GPU 메모리로 쏘아 올리는 기능을 전담합니다. 
void Mesh::CreateBuffers(ID3D12Device* device, const std::vector<Vertex>& vertices, const std::vector<std::uint16_t>& indices)
{ // 함수 시작
    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex); // 정점 배열의 총 바이트 크기입니다.
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t); // 인덱스 배열의 총 바이트 크기입니다.
    mIndexCount = (UINT)indices.size(); // 총 인덱스 개수를 저장합니다.

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD); // CPU에서 GPU로 복사할 수 있는 업로드 힙을 설정합니다.

    // 1. 정점 버퍼 할당 및 복사
    CD3DX12_RESOURCE_DESC vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize); // 정점 버퍼 스펙 정의
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vbDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mVertexBuffer));

    UINT8* pVertexDataBegin; // 메모리 포인터
    mVertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pVertexDataBegin)); // GPU 주소 받아오기
    memcpy(pVertexDataBegin, vertices.data(), vbByteSize); // C++ 데이터 덮어쓰기
    mVertexBuffer->Unmap(0, nullptr); // 주소 반환

    mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress(); // 뷰에 실제 주소 매핑
    mVertexBufferView.StrideInBytes = sizeof(Vertex); // 정점 1개 크기 지정
    mVertexBufferView.SizeInBytes = vbByteSize; // 전체 크기 지정

    // 2. 인덱스 버퍼 할당 및 복사
    CD3DX12_RESOURCE_DESC ibDesc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize); // 인덱스 버퍼 스펙 정의
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &ibDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mIndexBuffer));

    UINT8* pIndexDataBegin; // 메모리 포인터
    mIndexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pIndexDataBegin)); // GPU 주소 받아오기
    memcpy(pIndexDataBegin, indices.data(), ibByteSize); // C++ 데이터 덮어쓰기
    mIndexBuffer->Unmap(0, nullptr); // 주소 반환

    mIndexBufferView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress(); // 뷰에 실제 주소 매핑
    mIndexBufferView.Format = DXGI_FORMAT_R16_UINT; // 인덱스 포맷(16비트 정수) 지정
    mIndexBufferView.SizeInBytes = ibByteSize; // 전체 크기 지정
} // 함수 끝