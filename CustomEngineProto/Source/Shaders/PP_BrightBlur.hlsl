// [새로운 파일] 빛을 추출하고 뭉개는 셰이더 (PP_BrightBlur.hlsl) 
Texture2D gInputMap : register(t0); // 0번 도화지(원본)를 입력받습니다.
SamplerState gsSamLinearClamp : register(s0); // 부드럽게 읽어오는 샘플러입니다.

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 TexC : TEXCOORD;
};

float4 PSMain(VSOutput input) : SV_TARGET
{
    // 텍스처를 뭉갤 간격(오프셋)을 설정합니다. 숫자가 클수록 빛이 넓게 번집니다.
    float2 texOffset = float2(1.0f / 1280.0f, 1.0f / 720.0f) * 4.0f;
    float3 blurColor = float3(0.0f, 0.0f, 0.0f);
    
    // 블룸을 발생시킬 최소 밝기 기준점입니다. 이 값보다 밝아야 빛이 번집니다.
    float threshold = 0.7f;

    // 현재 픽셀을 중심으로 3x3 격자를 돌며 주변 색상을 수집합니다. (가장 단순한 Box Blur)
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            float2 offset = float2(x, y) * texOffset; // 샘플링할 오프셋 위치
            float3 c = gInputMap.Sample(gsSamLinearClamp, input.TexC + offset).rgb; // 색상 추출

            // RGB 색상을 이용해 사람 눈에 보이는 밝기(Luminance)를 계산합니다.
            float lum = dot(c, float3(0.2126f, 0.7152f, 0.0722f));
            
            // 밝기가 기준점(threshold)을 넘는 픽셀만 더해줍니다.
            if (lum > threshold)
            {
                blurColor += c;
            }
        }
    }
    
    // 9번 더했으므로 9로 나누어 평균값을 구합니다.
    blurColor /= 9.0f;

    // 번진 빛의 효과를 극대화하기 위해 밝기를 2배로 증폭시켜 반환합니다.
    return float4(blurColor * 2.0f, 1.0f);
}