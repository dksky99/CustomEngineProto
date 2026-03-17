//  [새로운 파일] 1. 원본 화면에서 밝은 픽셀만 추출하는 셰이더 (PP_BrightPass.hlsl) 
Texture2D gInputMap : register(t0); // 0번 임시 도화지(원본 3D 씬)를 입력받습니다.
SamplerState gsSamLinearClamp : register(s0); // 부드럽게 샘플링하는 상태 객체입니다.

struct VSOutput // 버텍스 셰이더에서 넘어온 구조체입니다.
{
    float4 Pos : SV_POSITION; // 화면 좌표입니다.
    float2 TexC : TEXCOORD; // UV 좌표입니다.
};

float4 PSMain(VSOutput input) : SV_TARGET // 픽셀 셰이더 메인 함수입니다.
{
    float4 color = gInputMap.Sample(gsSamLinearClamp, input.TexC); // 원본 색상을 읽어옵니다.
    
    // RGB 색상을 인간의 시각 인지 기준(Luminance) 단일 밝기 수치로 변환합니다.
    float lum = dot(color.rgb, float3(0.2126f, 0.7152f, 0.0722f));
    
    float threshold = 0.7f; // 블룸을 발생시킬 최소 밝기 기준점입니다.
    
    // 밝기가 임곗값을 넘으면 수치가 남고, 넘지 못하면 0.0이 됩니다.
    float contribution = max(0.0f, lum - threshold);
    
    // 남은 색상의 비율을 계산합니다. 분모가 0이 되지 않게 안전장치를 둡니다.
    contribution /= max(lum, 0.0001f);
    
    // 밝은 부분만 원본 색상을 유지하고 나머지는 완전한 검은색(0,0,0)으로 만듭니다.
    float3 brightColor = color.rgb * contribution;

    return float4(brightColor, 1.0f); // 추출된 빛 덩어리들을 반환합니다.
}