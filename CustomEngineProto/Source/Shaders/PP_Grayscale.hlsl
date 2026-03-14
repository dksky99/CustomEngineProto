//  [새로운 파일] 픽셀 셰이더: 흑백 필터 
Texture2D gInputMap : register(t0);
SamplerState gsSamLinearClamp : register(s0);

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 TexC : TEXCOORD;
};

float4 PSMain(VSOutput input) : SV_TARGET
{
    float4 color = gInputMap.Sample(gsSamLinearClamp, input.TexC);
    float gray = dot(color.rgb, float3(0.299f, 0.587f, 0.114f));
    return float4(gray, gray, gray, 1.0f);
}