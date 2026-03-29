#include "Texture.h" // 자신의 헤더 파일을 포함합니다.
#include "d3dx12.h" // 메모리 할당과 상태 전환 배리어를 돕는 헬퍼 헤더를 포함합니다.
#include <vector> // 픽셀 색상 데이터를 담을 가변 배열을 사용하기 위해 포함합니다.

// 윈도우 기본 내장 이미지 처리기(WIC)를 사용하기 위한 헤더와 라이브러리입니다.
#include <wincodec.h> // WIC(Windows Imaging Component) 기능이 들어있는 헤더를 포함합니다.
#pragma comment(lib, "windowscodecs.lib") // 컴파일러에게 WIC 라이브러리 파일을 연결하라고 지시합니다.


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

    //  텍스처 크기를 내가 짐작해서 할당하지 않고, DX12에게 256바이트 정렬이 적용된 '진짜 필요한 메모리 크기'를 물어봅니다! 
    UINT64 uploadBufferSize = 0;
    device->GetCopyableFootprints(&texDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD); // CPU가 덮어쓸 수 있는 시스템 메모리(업로드 힙)를 선택합니다.
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize); // 전송용 임시 버퍼의 크기를 설정합니다.

    // 업로드 전용 임시 메모리 공간을 생성합니다.
    device->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc, // 업로드 힙과 스펙을 전달합니다.
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mUploadHeap)); // 생성된 임시 버퍼를 변수에 담습니다.

    std::vector<uint8_t> textureData(texWidth * texHeight * texPixelSize); // CPU에서 색상을 계산해 둘 거대한 1차원 배열을 만듭니다.
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



//  윈도우 WIC 라이브러리를 이용하여 외부 이미지 파일(PNG, JPG 등)을 읽어오는 함수 전체 구현부입니다. 
bool Texture::LoadFromFile(const std::wstring& filepath, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) // 파일 로딩 구현부입니다.
{ // 함수 블록 시작입니다.
    // WIC는 COM(Component Object Model) 기반이므로 COM 라이브러리를 먼저 초기화해야 합니다.
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory; // WIC 기능을 총괄할 팩토리(공장) 포인터입니다.
    // 운영체제(OS)로부터 WIC 팩토리 객체의 인스턴스를 생성해달라고 요청합니다.
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));
    if (FAILED(hr)) return false; // 팩토리 생성에 실패하면 곧바로 false를 반환합니다.

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder; // 이미지 파일의 압축(PNG, JPG 등)을 해제할 디코더 포인터입니다.
    // 파일 경로를 넘겨주어 해당 이미지를 읽어들이고 디코더에 연결합니다.
    hr = wicFactory->CreateDecoderFromFilename(filepath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
    if (FAILED(hr)) return false; // 파일이 존재하지 않거나 경로가 틀리면 여기서 실패합니다!


    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame; // 이미지의 첫 번째 프레임(보통 사진은 1장)을 담을 포인터입니다.
    hr = decoder->GetFrame(0, &frame); // 디코더에서 0번째 원본 프레임을 추출합니다.
    if (FAILED(hr)) return false; // 프레임 추출 실패 시 반환합니다.

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter; // 이미지의 다양한 픽셀 포맷을 우리가 원하는 형식으로 변환할 컨버터입니다.
    hr = wicFactory->CreateFormatConverter(&converter); // 컨버터 객체를 공장에서 생성합니다.
    if (FAILED(hr)) return false; // 컨버터 생성 실패 시 반환합니다.

    //  우리가 읽어온 이미지가 RGB든 흑백이든 무조건 투명도를 포함한 32비트 RGBA(WICPixelFormat32bppRGBA)로 강제 변환합니다! 
    hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) return false; // 포맷 변환 실패 시 반환합니다.

    UINT width = 0, height = 0; // 이미지의 가로, 세로 픽셀 개수를 담을 변수입니다.
    converter->GetSize(&width, &height); // 컨버터에게 변환이 끝난 이미지의 진짜 사이즈를 물어봅니다.

    const UINT texPixelSize = 4; // 우리가 32비트 RGBA로 바꿨으므로 픽셀 1개당 무조건 4바이트를 차지합니다.
    const UINT rowPitch = width * texPixelSize; // 가로 한 줄의 총 바이트 용량을 계산합니다.
    const UINT imageSize = rowPitch * height; // 이미지 전체의 총 바이트 용량을 계산합니다.

    std::vector<uint8_t> imagePixels(imageSize); // 이미지의 모든 픽셀 데이터를 받아올 거대한 C++ 빈 배열을 만듭니다.
    // 컨버터에서 완전히 압축이 풀리고 RGBA로 변환된 순수 픽셀 데이터들을 배열로 싹 복사해옵니다.
    hr = converter->CopyPixels(nullptr, rowPitch, imageSize, imagePixels.data());
    if (FAILED(hr)) return false; // 픽셀 데이터 복사 실패 시 반환합니다.

    // --- 여기까지가 WIC를 이용한 [파일 열기 -> 압축 풀기 -> 픽셀 배열화] 완료입니다! ---
    // --- 아래부터는 추출한 픽셀 배열을 DX12 GPU 메모리로 쏘아 올리는 과정입니다. (체크무늬 함수와 원리가 똑같습니다) ---

    D3D12_RESOURCE_DESC texDesc = {}; // DX12 텍스처 스펙 구조체입니다.
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2D 평면 이미지입니다.
    texDesc.Alignment = 0; // 정렬 기본값입니다.
    texDesc.Width = width; // 이미지에서 읽어온 실제 가로 길이를 넣습니다.
    texDesc.Height = height; // 이미지에서 읽어온 실제 세로 길이를 넣습니다.
    texDesc.DepthOrArraySize = 1; // 1장입니다.
    texDesc.MipLevels = 1; // 밉맵 오프입니다.
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // WIC에서 RGBA로 변환했으므로 DX12 포맷도 R8G8B8A8로 정확히 맞춥니다.
    texDesc.SampleDesc.Count = 1; // 안티앨리어싱 오프입니다.
    texDesc.SampleDesc.Quality = 0; // 품질 기본값입니다.
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // 기본 배치입니다.
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // 기본 플래그입니다.

    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT); // 가장 빠른 GPU 전용 VRAM 힙을 선택합니다.
    device->CreateCommittedResource( // 실제 GPU 텍스처 메모리를 할당합니다.
        &defaultHeap, D3D12_HEAP_FLAG_NONE, &texDesc, // 스펙을 넘깁니다.
        D3D12_RESOURCE_STATE_COPY_DEST, // 데이터를 받아야 하므로 복사 목적지 상태로 생성합니다.
        nullptr, IID_PPV_ARGS(&mTexture)); // 멤버 변수에 텍스처를 저장합니다.


    // 어떤 해상도의 이미지가 들어오더라도 DX12의 256바이트 정렬 규약을 완벽히 맞춘 업로드 버퍼 크기를 얻어옵니다! 
    UINT64 requiredUploadSize = 0;
    device->GetCopyableFootprints(&texDesc, 0, 1, 0, nullptr, nullptr, nullptr, &requiredUploadSize);

    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD); // CPU에서 덮어쓸 임시 시스템 메모리(업로드 힙)를 선택합니다.
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(requiredUploadSize); // 이미지 전체 크기만큼 설정합니다.
    device->CreateCommittedResource( // 업로드 버퍼를 메모리에 생성합니다.
        &uploadHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc, // 스펙을 넘깁니다.
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mUploadHeap)); // 변수에 저장합니다.

    D3D12_SUBRESOURCE_DATA subResourceData = {}; // GPU로 전송할 데이터 덩어리 포장 구조체입니다.
    subResourceData.pData = imagePixels.data(); // 방금 WIC로 압축을 푼 C++ 이미지 배열의 첫 주소를 연결합니다.
    subResourceData.RowPitch = rowPitch; // 가로 한 줄의 바이트 크기를 알려줍니다.
    subResourceData.SlicePitch = imageSize; // 전체 이미지의 바이트 크기를 알려줍니다.

    // 렌더러가 준 명령서(cmdList)를 사용해 CPU의 업로드 힙 데이터를 GPU 텍스처 메모리로 초고속 복사하라는 명령을 적습니다.
    UpdateSubresources(cmdList, mTexture.Get(), mUploadHeap.Get(), 0, 0, 1, &subResourceData);

    // 복사가 완벽히 끝났으므로 셰이더가 자유롭게 읽을 수 있도록(PIXEL_SHADER_RESOURCE) 상태를 변경하는 배리어를 칩니다.
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE); // 상태 전환 설정입니다.
    cmdList->ResourceBarrier(1, &barrier); // 명령서에 배리어를 올립니다.

    return true; // 에러 없이 모든 PNG/JPG 파일 로딩과 GPU 전송이 끝났음을 알립니다!
} // 함수 블록 끝입니다.