//  [새로운 파일] 2. 가로 방향(Horizontal) 가우시안 블러 셰이더 (PP_BlurH.hlsl) 
Texture2D gInputMap : register(t0); // 방금 추출된 '빛 덩어리' 텍스처를 받습니다.
SamplerState gsSamLinearClamp : register(s0); // 부드러운 샘플러입니다.

struct VSOutput // 정점 출력 구조체입니다.
{
    float4 Pos : SV_POSITION; // 화면 좌표입니다.
    float2 TexC : TEXCOORD; // UV 좌표입니다.
};

float4 PSMain(VSOutput input) : SV_TARGET // 픽셀 셰이더 메인 함수입니다.
{
    // 가로 1픽셀 크기만큼의 간격을 계산합니다. (가로 해상도 1280 기준)
    float2 texOffset = float2(1.0f / 1280.0f, 0.0f);
    
    // 정규 분포(Gaussian) 곡선을 따르는 9개의 고정 가중치 배열입니다. (총합 = 1.0)
    float weight[9] = { 0.05f, 0.09f, 0.12f, 0.15f, 0.18f, 0.15f, 0.12f, 0.09f, 0.05f };
    
    float3 result = float3(0.0f, 0.0f, 0.0f); // 누적할 결과 색상 변수입니다.

    // 좌우로 4칸씩, 총 9칸의 픽셀을 순회하며 가로 방향으로만 섞어줍니다.
    for (int i = -4; i <= 4; ++i)
    {
        // 현재 UV에서 가로로 이동하며 색상을 읽습니다.
        float3 c = gInputMap.Sample(gsSamLinearClamp, input.TexC + float2(i, 0) * texOffset).rgb;
        // 해당 칸의 정해진 가중치(가운데일수록 높음)를 곱해서 누적합니다.
        result += c * weight[i + 4];
    }
    
    return float4(result, 1.0f); // 가로로 번진 이미지를 반환합니다.
}