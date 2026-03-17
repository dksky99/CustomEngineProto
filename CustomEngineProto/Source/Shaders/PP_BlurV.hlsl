//  [새로운 파일] 3. 세로 방향(Vertical) 가우시안 블러 셰이더 (PP_BlurV.hlsl) 
Texture2D gInputMap : register(t0); // 가로로 번진 텍스처를 입력받습니다.
SamplerState gsSamLinearClamp : register(s0); // 부드러운 샘플러입니다.

struct VSOutput // 정점 출력 구조체입니다.
{
    float4 Pos : SV_POSITION; // 화면 좌표입니다.
    float2 TexC : TEXCOORD; // UV 좌표입니다.
};

float4 PSMain(VSOutput input) : SV_TARGET // 픽셀 셰이더 메인 함수입니다.
{
    // 세로 1픽셀 크기만큼의 간격을 계산합니다. (세로 해상도 720 기준)
    float2 texOffset = float2(0.0f, 1.0f / 720.0f);
    
    // 가로와 완전히 동일한 9개의 고정 가중치 배열입니다.
    float weight[9] = { 0.05f, 0.09f, 0.12f, 0.15f, 0.18f, 0.15f, 0.12f, 0.09f, 0.05f };
    
    float3 result = float3(0.0f, 0.0f, 0.0f); // 누적할 결과 색상 변수입니다.

    // 위아래로 4칸씩, 총 9칸의 픽셀을 순회하며 세로 방향으로만 섞어줍니다.
    for (int i = -4; i <= 4; ++i)
    {
        // 현재 UV에서 세로로 이동하며 색상을 읽습니다.
        float3 c = gInputMap.Sample(gsSamLinearClamp, input.TexC + float2(0, i) * texOffset).rgb;
        // 해당 칸의 가중치를 곱해서 누적합니다.
        result += c * weight[i + 4];
    }
    
    // 가로+세로가 모두 뭉개진 완벽하고 영롱한 가우시안 블러 결과에 2배 밝기를 곱해 극대화합니다!
    return float4(result * 2.0f, 1.0f);
}