// ============================================================================
//  [15단계] 고급 포스트 프로세싱 전용 셰이더 (postprocess.hlsl) 
// 색수차(렌즈 왜곡)와 CRT 모니터 주사선 효과를 적용합니다!
// ============================================================================

// 1단계에서 100개의 큐브를 그렸던 '임시 도화지'를 텍스처(t0)로 받아옵니다!
Texture2D gOffscreenMap : register(t0);

// C++에서 부드러운 뭉개기(LINEAR)와 테두리 고정(CLAMP)으로 세팅한 샘플러입니다.
SamplerState gsSamLinearClamp : register(s0);

// 버텍스 셰이더에서 픽셀 셰이더로 넘겨줄 구조체입니다.
struct VSOutput
{
    float4 Pos : SV_POSITION; // 화면상에 그려질 2D 좌표입니다.
    float2 TexC : TEXCOORD; // 텍스처(임시 도화지)를 읽어올 UV 좌표입니다.
};

// ============================================================================
// [버텍스 셰이더 - 변경 없음]
// 화면을 덮는 거대한 삼각형을 만듭니다.
// ============================================================================
VSOutput VSMain(uint vid : SV_VertexID)
{
    VSOutput output;
    
    // 비트 연산을 통해 0, 1, 2번 정점을 화면 전체를 덮는 UV 좌표로 변환합니다.
    output.TexC = float2((vid << 1) & 2, vid & 2);
    
    // UV 좌표를 화면의 투영 좌표(-1 ~ 1)로 변환합니다.
    output.Pos = float4(output.TexC * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    
    return output;
}

// ============================================================================
// [픽셀 셰이더 -  새로운 마법 적용 ]
// ============================================================================
float4 PSMain(VSOutput input) : SV_TARGET
{
    // 화면 중앙(0.5, 0.5)에서 현재 픽셀이 얼마나 떨어져 있는지 방향과 거리를 구합니다.
    float2 centerOffset = input.TexC - 0.5f;
    float dist = length(centerOffset);

    // ------------------------------------------------------------------------
    //  1. 색수차 (Chromatic Aberration) 효과
    // 화면 테두리로 갈수록(dist가 클수록) 색상이 심하게 어긋나게 만듭니다.
    // ------------------------------------------------------------------------
    float aberrationAmount = dist * 0.03f; // 어긋나는 강도를 조절합니다. (0.03 = 3% 픽셀 엇갈림)
    
    // 빨간색(R)은 원래 위치보다 바깥쪽으로 밀어냅니다.
    float2 uvR = input.TexC + centerOffset * aberrationAmount;
    // 초록색(G)은 제자리에 둡니다.
    float2 uvG = input.TexC;
    // 파란색(B)은 원래 위치보다 안쪽으로 당깁니다.
    float2 uvB = input.TexC - centerOffset * aberrationAmount;
    
    // 각각 살짝 어긋난 3개의 UV 좌표로 텍스처를 3번 읽어옵니다!
    float r = gOffscreenMap.Sample(gsSamLinearClamp, uvR).r;
    float g = gOffscreenMap.Sample(gsSamLinearClamp, uvG).g;
    float b = gOffscreenMap.Sample(gsSamLinearClamp, uvB).b;
    
    // 3번 읽어온 RGB를 하나로 합칩니다. (테두리의 큐브들이 3D 안경을 안 쓴 것처럼 번져 보이게 됩니다)
    float3 finalColor = float3(r, g, b);

    // ------------------------------------------------------------------------
    //  2. 레트로 CRT 모니터 주사선 (Scanlines) 효과
    // ------------------------------------------------------------------------
    // 픽셀의 Y좌표에 sin 함수를 엄청나게 촘촘하게(800배) 곱해서 가로 줄무늬 진동을 만듭니다.
    // 결과값은 -1.0 ~ 1.0 사이로 진동합니다.
    float scanline = sin(input.TexC.y * 800.0f);
    
    // 진동 값을 살짝 약하게(0.04) 만들어서 원본 색상에서 빼줍니다. (어두운 가로줄 생성)
    finalColor -= scanline * 0.04f;

    // ------------------------------------------------------------------------
    //  3. 비네팅 (Vignette) 효과 유지
    // ------------------------------------------------------------------------
    // 거리가 0.2 ~ 1.0 사이일 때 테두리가 부드럽게 까매지도록 합니다.
    float vignette = smoothstep(1.0f, 0.2f, dist);
    finalColor *= vignette;
    
    // 최종 가공된 색상을 모니터로 송출합니다!
    return float4(finalColor, 1.0f);
}