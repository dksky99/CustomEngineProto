//  [새로운 파일] 모든 포스트 프로세싱이 공통으로 사용할 버텍스 셰이더 
// 효과가 10개든 100개든, 화면을 덮는 삼각형을 만드는 로직은 동일하므로 하나로 분리합니다.

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 TexC : TEXCOORD;
};

VSOutput VSMain(uint vid : SV_VertexID)
{
    VSOutput output;
    output.TexC = float2((vid << 1) & 2, vid & 2);
    output.Pos = float4(output.TexC * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}