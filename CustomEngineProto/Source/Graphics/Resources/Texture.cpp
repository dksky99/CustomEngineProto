#include "Texture.h" // 자신의 헤더 파일을 포함합니다.
#include "d3dx12.h" // 메모리 할당과 상태 전환 배리어를 돕는 헬퍼 헤더를 포함합니다.
#include <vector> // 픽셀 색상 데이터를 담을 가변 배열을 사용하기 위해 포함합니다.

//  렌더러의 BuildTexture 함수에 있던 코드가 이곳으로 깔끔하게 이사 왔습니다! 
void Texture::CreateCheckerboard(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) // 체크무늬 생성 함수의 구현부입니다.
{ // 함수 블록 시작입니다.
    const UINT texWidth = 256; // 생성할 텍스처의 가로 해상도(픽셀 수)를 256으로 설정합니다.
    const UINT texHeight = 256; // 생성할 텍스처의 세로 해상도를 256으로 설정합니다.
    const UINT texPixelSize = 4; // 픽셀 1개당 4바이트(R, G, B, A 각각 1바이트)를 차지함을 정의합니다.

    D3D12_RESOURCE_DESC texDesc = {}; // 텍스처의 스펙을 정의할 구조체를 0으로 초기화합니다.
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 이 리소스가 2D 평면 이미지임을 명시합니다.
    texDesc.Alignment = 0; // 메모리 정렬을 드라이버 기본값에 맡깁니다.
    texDesc.Width = texWidth; // 구조체에 가로 해상도를 입력합니다.
    texDesc.Height = texHeight; // 구조체에 세로 해상도를 입력합니다.
    texDesc.DepthOrArraySize = 1; // 텍스처가 단 1장뿐임을 명시합니다 (배열 아님).
    texDesc.MipLevels = 1; // 밉맵(거리별 저해상도 이미지)을 생성하지 않고 원본 1장만 씁니다.
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 색상 포맷을 8비트 RGBA로 설정합니다.
    texDesc.SampleDesc.Count = 1; // 안티앨리어싱(MSAA)을 적용하지 않습니다.
    texDesc.SampleDesc.Quality = 0; // 안티앨리어싱 품질을 0으로 둡니다.
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // 텍스처 내부 메모리 배치를 드라이버가 알아서 최적화하도록 둡니다.
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // 특별한 제약이나 허가 플래그 없이 기본 텍스처로 만듭니다.

    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT); // GPU가 가장 빠르게 읽을 수 있는 전용 메모리(VRAM) 힙을 선택합니다.

    // 정의한 스펙을 바탕으로 실제 GPU 전용 메모리에 텍스처 공간을 할당합니다.
    device->CreateCommittedResource(
        &defaultHeap, D3D12_HEAP_FLAG_NONE, &texDesc, // 메모리 종류와 스펙을 전달합니다.
        D3D12_RESOURCE_STATE_COPY_DEST, // 처음에는 CPU의 데이터를 복사받을 목적지(COPY_DEST) 상태로 생성합니다.
        nullptr, IID_PPV_ARGS(&mTexture)); // 생성된 텍스처 객체를 멤버 변수에 쏙 담습니다.

    const UINT64 uploadBufferSize = texWidth * texHeight * texPixelSize; // 총 몇 바이트를 복사해야 하는지 계산합니다.
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD); // CPU가 덮어쓸 수 있는 시스템 메모리(업로드 힙)를 선택합니다.
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize); // 전송용 임시 버퍼의 크기를 설정합니다.

    // 업로드 전용 임시 메모리 공간을 생성합니다.
    device->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc, // 업로드 힙과 스펙을 전달합니다.
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mUploadHeap)); // 생성된 임시 버퍼를 변수에 담습니다.

    std::vector<uint8_t> textureData(uploadBufferSize); // CPU에서 색상을 계산해 둘 거대한 1차원 배열을 만듭니다.
    for (UINT y = 0; y < texHeight; y++) // 텍스처의 세로 픽셀 수만큼 루프를 돕니다.
    { // 외부 루프 시작입니다.
        for (UINT x = 0; x < texWidth; x++) // 텍스처의 가로 픽셀 수만큼 루프를 돕니다.
        { // 내부 루프 시작입니다.
            bool isWhite = ((x / 32) % 2) == ((y / 32) % 2); // 32픽셀 단위로 체크무늬 패턴을 계산하는 수학 공식입니다.
            uint8_t color = isWhite ? 255 : 50; // 패턴에 따라 밝은 흰색(255) 또는 짙은 회색(50)을 결정합니다.

            UINT index = (y * texWidth + x) * texPixelSize; // 2차원 좌표(x, y)를 1차원 배열의 인덱스로 변환합니다.
            textureData[index + 0] = color; // Red 채널에 색상을 넣습니다.
            textureData[index + 1] = color; // Green 채널에 색상을 넣습니다.
            textureData[index + 2] = 255; // Blue 채널은 255로 고정하여 전체적으로 푸른빛이 돌게 만듭니다.
            textureData[index + 3] = isWhite ? 180 : 50; // 반투명도를 위해 Alpha 채널 값을 조정합니다.
        } // 내부 루프 끝입니다.
    } // 외부 루프 끝입니다.

    D3D12_SUBRESOURCE_DATA subResourceData = {}; // GPU로 전송할 데이터 덩어리를 포장할 구조체입니다.
    subResourceData.pData = textureData.data(); // 우리가 방금 색칠한 CPU 배열의 시작 주소를 연결합니다.
    subResourceData.RowPitch = texWidth * texPixelSize; // 이미지의 가로 한 줄이 총 몇 바이트인지 알려줍니다.
    subResourceData.SlicePitch = subResourceData.RowPitch * texHeight; // 이미지 전체가 총 몇 바이트인지 알려줍니다.

    // 렌더러가 넘겨준 커맨드 리스트(명령서)를 사용해, CPU 메모리(업로드 힙)의 데이터를 GPU 메모리(텍스처)로 고속 복사하도록 명령을 기록합니다.
    UpdateSubresources(cmdList, mTexture.Get(), mUploadHeap.Get(), 0, 0, 1, &subResourceData);

    // 복사가 끝난 텍스처를, 셰이더가 마음껏 읽어갈 수 있는 '픽셀 셰이더 리소스(SRV)' 상태로 전환하라는 배리어 명령을 기록합니다.
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE); // 상태 전환 설정입니다.
    cmdList->ResourceBarrier(1, &barrier); // 커맨드 리스트에 배리어를 올립니다.
} // 함수 블록 끝입니다.