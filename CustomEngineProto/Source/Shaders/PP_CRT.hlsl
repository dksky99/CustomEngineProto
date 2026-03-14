//  [새로운 파일] 픽셀 셰이더: 레트로 CRT 및 비네팅 필터 
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
    
    // CRT 스캔라인
    float scanline = sin(input.TexC.y * 800.0f);
    color.rgb -= scanline * 0.04f;

    // 비네팅
    float dist = length(input.TexC - 0.5f);
    float vignette = smoothstep(1.0f, 0.2f, dist);
    color.rgb *= vignette;
    
    return float4(color.rgb, 1.0f);
}