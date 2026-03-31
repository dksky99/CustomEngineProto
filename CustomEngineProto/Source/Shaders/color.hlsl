// ============================================================================
//    [최종 수정본] 빛과 그림자를 그리는 메인 셰이더 (color.hlsl)   
// ============================================================================

//    1. 매 프레임 단 1번만 바뀌는 '공통 데이터 (Pass Constants)' - b0 레지스터   
cbuffer cbPass : register(b0)
{
    float4x4 gViewProj; //    카메라의 View * Projection 행렬입니다.
    float3 gLightDir; //    공통 빛 방향입니다.
    float gTotalTime; //    텍스처를 움직이게 만들 '현재 시간'입니다.
    float4 gLightColor; //    공통 빛 색상입니다.
    float3 gEyePosW; //    스페큘러(반사광)를 계산하기 위한 '카메라의 현재 월드 위치'입니다.
    float pad; //    16바이트 정렬을 맞추기 위한 빈칸입니다.
    float4x4 gLightTransform; //    3D 정점을 '태양(빛)의 눈으로 바라본 UV 좌표'로 변환해 주는 그림자 마법 행렬입니다!
    
    // C++에서 넘겨준 태양의 뷰/투영 행렬을 받을 변수를 추가합니다! 
    float4x4 gLightViewProj;
    
    
     //  C++ 구조체와 1비트도 틀리지 않게 점광원 변수 5줄을 선언합니다. 
    float3 gPointLightPosW;
    float gPointLightFalloffStart;
    float4 gPointLightColor;
    float gPointLightFalloffEnd;
    float3 pad2;
    
}; 

//  C++에서 쏴준 '배치 그룹의 시작 번호'를 받을 상수 버퍼(b1) 공간을 오픈합니다! 
cbuffer cbBatch : register(b1)
{
    uint gStartInstanceLocation; // "이번 그룹은 1번(지구)부터 시작해!"
    float3 pad3; // 16바이트 정렬을 위한 패딩
};

//    큐브 하나가 가질 데이터를 구조체로 정의합니다.   
struct InstanceData
{
    float4x4 World;
    float4 BaseColor; // C++에서 보낸 색상
    float3 Emissive; // C++에서 보낸 발광
    float Roughness; // 추가됨!
    float Metallic; // 추가됨!
    float3 pad; // 추가됨!
};
//    100개의 큐브 위치 데이터가 담긴 배열(명부)을 통째로 t1 레지스터로 받습니다!   
StructuredBuffer<InstanceData> gInstanceData : register(t1);

//    텍스처 이미지 데이터(t0)를 선언합니다.   
Texture2D gDiffuseMap : register(t0);

//    태양 시점에서 렌더링 된 거대한 '섀도우 맵(깊이 텍스처)'을 3번 칸(t2)으로 가져옵니다!   
Texture2D gShadowMap : register(t2);

// C++에서 5번 슬롯에 꽂아준 노멀맵 이미지를 t3 레지스터로 받습니다! 
Texture2D gNormalMap : register(t3);

//    색상 텍스처를 선명하게 읽는 샘플러(s0)입니다.   
SamplerState gsSamPointWrap : register(s0);

//    섀도우 맵 전용 특수 샘플러(s1)입니다. 깊이 값을 비교해서 그림자 여부를 뱉어냅니다.   
SamplerComparisonState gsSamShadow : register(s1);

//    정점 셰이더(Vertex Shader)로 들어올 입력 데이터의 형식을 정의하는 구조체입니다.   
struct VSInput
{
    float3 Pos : POSITION; //    3차원 위치(Position) 데이터입니다. 
    float4 Color : COLOR; //    4차원 색상(Color) 데이터입니다.
    float3 Normal : NORMAL; //    이 꼭짓점이 바라보는 방향(법선 벡터)입니다.
    float2 TexC : TEXCOORD; //    텍스처 UV 좌표(2D)입니다.
    // C++에서 넘어오는 접선 벡터입니다!
    float3 Tangent : TANGENT;
    uint instanceID : SV_InstanceID; //    GPU가 "지금 내가 몇 번째 큐브를 그리고 있지?"를 알 수 있게 해주는 마법의 시스템 변수입니다.
};

//    정점 셰이더가 픽셀 셰이더로 넘겨줄 출력 데이터의 구조체입니다.   
struct PSInput
{
    float4 Pos : SV_POSITION; //    화면상에 그려질 최종 2D 좌표 위치입니다.
    float3 PosW : POSITION; //    픽셀 셰이더에서 반사 각도를 계산하기 위한 '월드 공간 위치'입니다.
    float4 Color : COLOR; //    색상 정보입니다.
    float3 NormalW : NORMAL; //    월드 공간으로 변환된 법선 벡터입니다.
    float2 TexC : TEXCOORD; //    텍스처 샘플링을 위한 UV 좌표입니다.
    float4 ShadowPos : TEXCOORD1; //    픽셀 셰이더에게 넘겨줄 "태양 시점에서의 내 그림자 텍스처 UV 좌표"입니다!
    
     //픽셀 셰이더로 넘겨줄 발광(Glow) 데이터를 위한 배달 상자 추가 
    float3 Emissive : TEXCOORD5;
    
     //  거칠기와 금속성을 픽셀 셰이더로 배달할 변수(주머니)를 추가합니다! 
    float Roughness : TEXCOORD6;
    float Metallic : TEXCOORD7;
    
      // 월드 공간으로 변환된 접선 벡터를 픽셀 셰이더로 넘겨줍니다.
    float3 TangentW : TANGENT;
};

//    픽셀 셰이더의 최종 출력물을 정의하는 구조체입니다.   
struct PSOutput
{
    float4 Color : SV_TARGET0; //    메인 도화지에 출력될 최종 색상입니다. 
};

// 섀도우 맵(그림자)을 그릴 때만 호출될 '그림자 전용 버텍스 셰이더'를 신설합니다! 
struct ShadowVSOutput
{
    float4 Pos : SV_POSITION;
};

ShadowVSOutput ShadowVS(VSInput input)
{
    ShadowVSOutput output;
    
    // 바보 변수(0)에 C++이 넘겨준 진짜 시작 번호를 더해서 '진짜 인덱스 번호'를 찾습니다! 
    uint realInstanceID = input.instanceID + gStartInstanceLocation;
    
   // 진짜 인덱스 번호로 내 월드 행렬을 꺼냅니다.
    float4x4 worldMat = gInstanceData[realInstanceID].World;
    
    float4 posW = float4(input.Pos, 1.0f);
    posW = mul(posW, worldMat);
    
    //  핵심: 일반 카메라(gViewProj)가 아니라 태양의 눈(gLightViewProj)으로 화면을 압축하여 깊이를 기록합니다! 
    output.Pos = mul(posW, gLightViewProj);
    
    return output;
}
// 

//    1. 버텍스 셰이더의 메인 함수입니다.   
PSInput VSMain(VSInput input)
{
    PSInput output;
    
     //  여기서도 바보 변수를 진짜 인덱스로 고쳐서 명부를 조회합니다! 
    uint realInstanceID = input.instanceID + gStartInstanceLocation;
    
    float4x4 worldMat = gInstanceData[realInstanceID].World;
    
    float4 posW = float4(input.Pos, 1.0f);
    posW = mul(posW, worldMat); //    큐브를 3D 공간 각자의 위치로 보냅니다.
    
    output.PosW = posW.xyz;
    output.Pos = mul(posW, gViewProj); //    모니터 화면 좌표로 튕겨냅니다.
    
    output.NormalW = mul(input.Normal, (float3x3) worldMat);
    
    
    // 접선(Tangent)도 행렬에 곱해 월드로 변환합니다!
    output.TangentW = mul(input.Tangent, (float3x3) worldMat);
    
     // 색상과 발광 데이터도 '진짜 인덱스'로 꺼내옵니다.
    output.Color = input.Color * gInstanceData[realInstanceID].BaseColor;
    output.Emissive = gInstanceData[realInstanceID].Emissive;
     //  버텍스 셰이더에서 미리 꺼낸 다음, 픽셀 셰이더로 보낼 배달 상자(output)에 넣습니다! 
    output.Roughness = gInstanceData[realInstanceID].Roughness;
    output.Metallic = gInstanceData[realInstanceID].Metallic;
    
    // 행성이 대각선으로 흘러내리게 하던 UV 애니메이션을 지우고, 순수하게 C++의 자전 방향으로만 돌게 둡니다! 
    output.TexC = input.TexC;
    
    output.ShadowPos = mul(posW, gLightTransform); //    그림자 맵에서의 내 위치(UV)를 계산합니다.
    
    return output;
}


// ============================================================================
//  [마법의 공식] 물리 기반 렌더링(PBR) 3대 수학 함수 
// ============================================================================
static const float PI = 3.14159265359f; // 파이(원주율) 상수입니다. 빛이 퍼지는 양을 계산할 때 씁니다.

// 1. 미세표면 분포 함수 (D): 표면의 미세한 굴곡이 빛을 얼마나 한 곳으로 모아주는지(광택의 선명도) 계산합니다.
float D_GGX(float NdotH, float roughness) // 법선과 하프벡터의 내적, 거칠기를 받습니다.
{ // D 함수 시작
    float alpha = roughness * roughness; // 시각적 자연스러움을 위해 거칠기를 제곱하여 사용합니다.
    float alpha2 = alpha * alpha; // 공식에 맞게 한 번 더 제곱합니다.
    float denom = NdotH * NdotH * (alpha2 - 1.0f) + 1.0f; // 분모 부분의 식을 계산합니다.
    return alpha2 / (PI * denom * denom + 0.0001f); // 최종 GGX 공식을 반환합니다. (0으로 나누기 방지 0.0001f)
} // D 함수 끝

// 2. 기하 감쇠 함수 (G): 표면의 미세한 굴곡들이 서로 빛을 가려서 그림자 지는 현상을 시뮬레이션합니다.
float G_SchlickGGX(float NdotV, float NdotL, float roughness) // 시선 내적, 조명 내적, 거칠기를 받습니다.
{ // G 함수 시작
    float r = roughness + 1.0f; // 직접 조명에 맞는 거칠기 조정을 합니다.
    float k = (r * r) / 8.0f; // k 상수를 계산합니다.
    float gV = NdotV / (NdotV * (1.0f - k) + k + 0.0001f); // 시선 방향의 가려짐을 계산합니다.
    float gL = NdotL / (NdotL * (1.0f - k) + k + 0.0001f); // 조명 방향의 가려짐을 계산합니다.
    return gV * gL; // 두 가려짐 확률을 곱해 최종 그림자 율을 반환합니다.
} // G 함수 끝

// 3. 프레넬 함수 (F): 물체를 비스듬히 볼수록 거울처럼 반사율이 급격히 높아지는 물리 현상을 시뮬레이션합니다.
float3 F_Schlick(float HdotV, float3 F0) // 시선과 하프벡터의 내적, 기본 반사율을 받습니다.
{ // F 함수 시작
    // 정면에서 본 반사율(F0)에다가, 각도에 따라 반사율이 1.0(하얀색)으로 올라가는 곡선을 더해 반환합니다.
    return F0 + (1.0f - F0) * pow(saturate(1.0f - HdotV), 5.0f);
} // F 함수 끝
// ============================================================================
//    2. 픽셀 셰이더의 메인 함수입니다.   
PSOutput PSMain(PSInput input)
{
    PSOutput output;

    float4 texColor = gDiffuseMap.Sample(gsSamPointWrap, input.TexC);

      //   [안전 장치 1] 만약 외부 텍스처(Test.png)가 너무 까맣게 로드되었다면, 강제로 밝은 회색으로 살려냅니다!  
    if (length(texColor.rgb) < 0.1f){texColor.rgb = float3(0.8f, 0.8f, 0.8f);}
      //  텍스처 색상에 위에서 전달받은 큐브 고유의 색상을 곱해 최종 껍데기 색을 만듭니다. 
    float4 baseColor = texColor * input.Color;
    
     //  [핵심 마법: TBN 행렬 조립과 법선 찌그러뜨리기!] 
    // 1. 노멀맵 이미지에서 RGB 색상을 뽑아옵니다.
    float3 normalMapSample = gNormalMap.Sample(gsSamPointWrap, input.TexC).rgb;
    
      
    
  
    
    // 2. 부드러운 원래 법선(N)과 접선(T)을 가져옵니다. 오류 방지를 위해 길이를 검사합니다.
      //   [핵심 변경점] 외부 OBJ 파일에 법선(vn)이 없어 (0,0,0)이 들어왔을 때 발생하는 NaN(검은 화면) 버그를 완벽히 차단합니다!  
    float3 normal = input.NormalW;
    if (length(normal) < 0.01f)
        // 법선이 없다면 강제로 위쪽(Y축)을 바라보게 만들어 수학 에러를 방지합니다.
        normal = float3(0.0f, 1.0f, 0.0f);
    else
        // 법선이 정상적으로 존재할 때만 길이를 1로 맞춥니다.
        normal = normalize(normal);
    
    float3 T = input.TangentW;
    if (length(T) < 0.01f)
        T = float3(1.0f, 0.0f, 0.0f);
    
    // 3. T벡터와 N벡터가 수직이 아닐 수 있으므로 직교화시킵니다 (Gram-Schmidt).
    T = normalize(T - dot(T, normal) * normal);
    
    // 4. 두 벡터에 모두 수직인 3번째 종법선(B)을 외적으로 구합니다.
    float3 B = cross(normal, T);
    
    // 5. 3개의 축(T, B, N)으로 3D 행렬을 조립합니다!
    float3x3 TBN = float3x3(T, B, normal);
    
    // 6. 노멀맵 색상 [0 ~ 1] 범위를 방향 벡터 [-1 ~ 1] 범위로 압축 해제합니다.
    float3 bumpedNormalW = normalMapSample * 2.0f - 1.0f;
    
    // 7. 이 가짜 방향에 TBN 행렬을 곱하면, 둥근 표면의 각도에 딱 맞게 진짜처럼 찌그러진 새로운 법선이 완성됩니다!
    bumpedNormalW = normalize(mul(bumpedNormalW, TBN));
    
    // =====================================================================
    // [PBR 초기 변수 세팅] 
    // ====================================================================
   //  에러의 주범 해결! 배열에서 강제로 꺼내지 않고, 배달 온 상자(input)에서 바로 꺼내 씁니다! 
    // 0.0이 되면 수학 연산 중 0으로 나누기 에러가 나므로 거칠기의 최솟값을 0.05로 제한합니다.
    float roughness = max(input.Roughness, 0.05f);
    float metallic = saturate(input.Metallic); // 0.0 ~ 1.0 사이로 자릅니다.
    
    // 조명 계산에 쓸 최종 법선(N)을 찌그러진 노멀맵 법선으로 교체합니다!
    float3 N = bumpedNormalW;
    // 내 픽셀에서 카메라(눈)를 향하는 방향 벡터(V)를 구합니다.
    float3 V = normalize(gEyePosW - input.PosW);
    // 법선(N)과 시선(V)의 내적 값(각도)을 구합니다. 음수가 되지 않게 max로 막아줍니다.
    float NdotV = max(dot(N, V), 0.0f);
    
    // F0: 기본 반사율입니다. 비금속 물질은 보통 4%(0.04)의 빛만 반사합니다.
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    // 금속성(metallic)이 높을수록 기본 반사율이 자신의 텍스처 색상(baseColor)을 따라가게 섞어줍니다.
    F0 = lerp(F0, baseColor.rgb, metallic);
    
    // 픽셀이 최종적으로 받게 될 빛의 에너지를 차곡차곡 누적할 변수(Lo)입니다. 초기값은 까만색입니다.
    float3 Lo = float3(0.0f, 0.0f, 0.0f);
    // =====================================================================

  // PCF (Percentage-Closer Filtering) 부드러운 그림자 연산 
    float shadowFactor = 0.0f;
    input.ShadowPos.xyz /= input.ShadowPos.w;
    
    // 2048 해상도 섀도우 맵 기준 1픽셀의 간격입니다.
    float dx = 1.0f / 2048.0f;
    
    // 픽셀 주변 3x3(9개) 격자를 돌며 그림자 여부를 검사하고 평균을 냅니다.
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            float2 offset = float2(x, y) * dx;
            shadowFactor += gShadowMap.SampleCmpLevelZero(gsSamShadow, input.ShadowPos.xy + offset, input.ShadowPos.z - 0.003f).r;
        }
    }
    shadowFactor /= 9.0f; // 9번 더했으므로 평균값 산출
    
    shadowFactor = max(shadowFactor, step(1.0f, input.ShadowPos.z));
    shadowFactor = shadowFactor * 0.5f + 0.5f;

      //  [문제 1 해결] 누락되어 화면을 까맣게 만들었던 '방향광(우주 배경빛) PBR 연산'을 복구합니다!  
    float3 L = normalize(-gLightDir); // 빛이 날아오는 방향의 반대(픽셀에서 빛을 향하는 방향)를 구합니다.
    float3 H = normalize(V + L); // 시선(V)과 빛(L)의 정확히 중간을 가리키는 하프(Half) 벡터를 구합니다.

    float NdotL = max(dot(N, L), 0.0f); // 법선과 빛 방향의 내적 (빛을 정면으로 받을수록 밝아짐)
    float NdotH = max(dot(N, H), 0.0f); // 법선과 하프 벡터의 내적 (정반사 계산용)
    float HdotV = max(dot(H, V), 0.0f); // 하프 벡터와 시선의 내적 (프레넬 효과 계산용)

    float NDF = D_GGX(NdotH, roughness); // [PBR 1] 미세 굴곡 분포: 매끄러울수록 빛을 한 곳으로 모아줍니다.
    float G_val = G_SchlickGGX(NdotV, NdotL, roughness); // [PBR 2] 기하 감쇠: 표면의 미세한 돌기가 스스로 빛을 가리는 현상입니다.
    float3 F = F_Schlick(HdotV, F0); // [PBR 3] 프레넬 효과: 비스듬히 볼수록 거울처럼 반사율이 치솟는 현상입니다.

    float3 numerator = NDF * G_val * F; // 3대 공식을 모두 곱하여 반사되는 빛의 분자를 만듭니다.
    float denominator = 4.0f * NdotV * NdotL + 0.0001f; // 에너지 보존 법칙을 지키기 위한 분모입니다. (0 나누기 에러 방지)
    float3 specular = numerator / denominator; // 최종적으로 튕겨 나가는 빛(정반사광)의 에너지가 산출되었습니다.

    float3 kS = F; // 거울처럼 튕겨 나간 빛의 비율을 프레넬 값(F)으로 정합니다.
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS; // 전체 빛(1.0)에서 튕겨 나간 빛(kS)을 빼면, 표면에 스며드는 빛(난반사광)의 비율이 됩니다.
    kD *= 1.0f - metallic; // 금속(Metallic)은 빛을 스며들지 못하게 튕겨내므로, 금속성이 높을수록 난반사광을 0으로 지웁니다.

    // 스며든 빛과 튕겨 나간 빛을 합치고, 빛의 본래 색상과 각도, 그림자 여부를 곱해 최종 빛 에너지 저장소(Lo)에 누적합니다!
    Lo += (kD * baseColor.rgb / PI + specular) * gLightColor.rgb * NdotL * shadowFactor;


     // 대기권 산란(Rim Lighting / Fresnel) 연산 
    // 시선과 픽셀의 방향이 수직(테두리)에 가까울수록 수치가 1.0으로 올라갑니다.
     //  테두리 빛도 찌그러진 노멀맵을 사용합니다!
    //  [문제 2 해결] 에러를 내던 옛날 변수 'viewVec'을 새로운 PBR 시선 변수인 'V'로 완벽하게 교체했습니다!  
    float rim = 1.0f - saturate(dot(V, bumpedNormalW));

    // 0.6 미만은 0으로 날려버려, 아주 얇은 외곽선에만 빛이 맺히도록 깎아냅니다.
    rim = smoothstep(0.6f, 1.0f, rim);

    // 텍스처 고유의 색상에 맞춰 테두리 빛을 뿜어냅니다! (지구는 파란 테두리, 달은 회색 테두리)
    float3 rimColor = baseColor.rgb * rim * 1.5f;



     // --- 2.  점광원 (Point Light) 연산 시작!  ---
    // 픽셀의 현재 위치에서 점광원(태양)까지의 3D 화살표(벡터)를 만듭니다.
    float3 pointLightVec = gPointLightPosW - input.PosW;
    // 화살표의 길이 = 빛의 근원지까지의 실제 거리(미터)입니다.
    float d = length(pointLightVec);

    // 거리가 소멸 거리(FalloffEnd)보다 가까울 때만 셰이더 연산을 합니다! (최적화) if문일지라도 if문안의 내용이 더 무겁다면 if문을 써야한다.
    if (d < gPointLightFalloffEnd)
    {
        //  [수정점] 옛날 낡은 공식을 지우고, 점광원도 PBR 공식으로 완벽하게 업그레이드했습니다! 
        float3 L_pt = pointLightVec / d; // 화살표를 거리로 나누어 길이가 1인 방향 벡터로 정규화합니다.
        float3 H_pt = normalize(V + L_pt); // 여기서도 에러 원인이었던 viewVec 대신 V를 사용합니다!

        float NdotL_pt = max(dot(N, L_pt), 0.0f); // 점광원 방향과의 내적
        float NdotH_pt = max(dot(N, H_pt), 0.0f); // 점광원 하프 벡터와의 내적
        float HdotV_pt = max(dot(H_pt, V), 0.0f); // 에러 방지를 위해 V 사용

        float NDF_pt = D_GGX(NdotH_pt, roughness); // 점광원의 굴곡 분포 계산
        float G_pt = G_SchlickGGX(NdotV, NdotL_pt, roughness); // 점광원의 기하 감쇠 계산
        float3 F_pt = F_Schlick(HdotV_pt, F0); // 점광원의 프레넬 반사율 계산

        float3 num_pt = NDF_pt * G_pt * F_pt; // 점광원 정반사 분자
        float den_pt = 4.0f * NdotV * NdotL_pt + 0.0001f; // 점광원 정반사 분모
        float3 spec_pt = num_pt / den_pt; // 점광원 최종 튕겨 나간 빛(정반사)

        float3 kS_pt = F_pt; // 점광원의 튕겨 나간 비율
        float3 kD_pt = float3(1.0f, 1.0f, 1.0f) - kS_pt; // 점광원의 스며든 비율
        kD_pt *= 1.0f - metallic; // 금속은 빛을 흡수하지 않게 차단

        // [감쇠(Attenuation) 수학 공식] 거리가 멀어질수록 빛이 사그라드는 비율을 계산합니다. 
        // 거리가 FalloffStart보다 작으면 1.0(최대), End에 다다르면 0.0으로 부드럽게 감소시킵니다.
        float attenuation = saturate((gPointLightFalloffEnd - d) / (gPointLightFalloffEnd - gPointLightFalloffStart));
        
        // 거리에 따라 감쇠된 진짜 점광원 빛 에너지를 구합니다.
        float3 radiance = gPointLightColor.rgb * attenuation;

        // 점광원의 에너지 역시 빛 저장소(Lo)에 차곡차곡 누적(+=)해 줍니다!
        Lo += (kD_pt * baseColor.rgb / PI + spec_pt) * radiance * NdotL_pt;
    }
    // -------------------------------------------------------------

    //  [문제 3 해결] 에러를 내뿜던 누락된 ambient(환경광) 변수를 새로 선언해 줍니다!  
    float3 ambient = float3(0.05f, 0.05f, 0.08f) * baseColor.rgb; // 어두운 우주에 아주 약하게 깔린 기본 빛입니다.

    //  환경광 + 방향광/점광원 PBR 에너지 총합(Lo) + 테두리 빛 + 자체 발광(Emissive) 
    float3 finalColor = ambient + Lo + rimColor + input.Emissive;

    output.Color = float4(finalColor, 1.0f);

    return output;
}