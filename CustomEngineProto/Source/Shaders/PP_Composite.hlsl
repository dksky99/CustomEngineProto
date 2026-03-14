//  [새로운 파일] 두 화면을 더하여 완성하는 셰이더 (PP_Composite.hlsl) 
Texture2D gOriginalMap : register(t0); // 0번 도화지 (원본 3D 씬)
Texture2D gBloomMap : register(t1); // 1번 도화지 (BrightBlur를 거친 빛 무리)
SamplerState gsSamLinearClamp : register(s0); // 부드럽게 읽어오는 샘플러입니다.

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 TexC : TEXCOORD;
};

float4 PSMain(VSOutput input) : SV_TARGET
{
    // 1. 원본 화면 색상을 읽어옵니다.
    float3 original = gOriginalMap.Sample(gsSamLinearClamp, input.TexC).rgb;

    // 2. 번져있는 빛(블룸) 색상을 읽어옵니다.
    float3 bloom = gBloomMap.Sample(gsSamLinearClamp, input.TexC).rgb;

    // 3. 두 색상을 더합니다. (이것이 가장 기본적인 Additive 렌더링입니다)
    float3 finalColor = original + bloom;

    // 4. 톤 매핑 (Tone Mapping - Reinhard)
    // 빛이 너무 많이 더해져 색상이 하얗게 타버리는(Blow out) 현상을 막기 위해, 수치를 0.0 ~ 1.0 사이로 부드럽게 압축합니다.
    finalColor = finalColor / (finalColor + float3(1.0f, 1.0f, 1.0f));

    return float4(finalColor, 1.0f);
}