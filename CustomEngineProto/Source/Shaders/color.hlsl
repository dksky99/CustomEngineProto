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

//    큐브 하나가 가질 데이터를 구조체로 정의합니다.   
struct InstanceData
{
    float4x4 World;
    float4 BaseColor; // C++에서 보낸 색상
    float3 Emissive; // C++에서 보낸 발광
    float pad;
};
//    100개의 큐브 위치 데이터가 담긴 배열(명부)을 통째로 t1 레지스터로 받습니다!   
StructuredBuffer<InstanceData> gInstanceData : register(t1);

//    텍스처 이미지 데이터(t0)를 선언합니다.   
Texture2D gDiffuseMap : register(t0);

//    태양 시점에서 렌더링 된 거대한 '섀도우 맵(깊이 텍스처)'을 3번 칸(t2)으로 가져옵니다!   
Texture2D gShadowMap : register(t2);

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
    float4x4 worldMat = gInstanceData[input.instanceID].World;
    
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
    
    float4x4 worldMat = gInstanceData[input.instanceID].World; //    내 번호표에 맞는 월드 행렬을 빼옵니다.
    
    float4 posW = float4(input.Pos, 1.0f);
    posW = mul(posW, worldMat); //    큐브를 3D 공간 각자의 위치로 보냅니다.
    
    output.PosW = posW.xyz;
    output.Pos = mul(posW, gViewProj); //    모니터 화면 좌표로 튕겨냅니다.
    
    output.NormalW = mul(input.Normal, (float3x3) worldMat);
    // 큐브마다 고유하게 전달된 색상과 발광 데이터를 출력 상자에 담습니다! 
    output.Color = input.Color * gInstanceData[input.instanceID].BaseColor;
    output.Emissive = gInstanceData[input.instanceID].Emissive;
    
    output.TexC = input.TexC + float2(gTotalTime * 0.1f, gTotalTime * 0.1f); //    텍스처 애니메이션!
    
    output.ShadowPos = mul(posW, gLightTransform); //    그림자 맵에서의 내 위치(UV)를 계산합니다.
    
    return output;
}

//    2. 픽셀 셰이더의 메인 함수입니다.   
PSOutput PSMain(PSInput input)
{
    PSOutput output;

    float4 texColor = gDiffuseMap.Sample(gsSamPointWrap, input.TexC);

      //   [안전 장치 1] 만약 외부 텍스처(Test.png)가 너무 까맣게 로드되었다면, 강제로 밝은 회색으로 살려냅니다!  
    if (length(texColor.rgb) < 0.1f)
    {
        texColor.rgb = float3(0.8f, 0.8f, 0.8f);
    }
    
      //  텍스처 색상에 위에서 전달받은 큐브 고유의 색상을 곱해 최종 껍데기 색을 만듭니다. 
    float4 baseColor = texColor * input.Color;
    
      //   [핵심 변경점] 외부 OBJ 파일에 법선(vn)이 없어 (0,0,0)이 들어왔을 때 발생하는 NaN(검은 화면) 버그를 완벽히 차단합니다!  
    float3 normal = input.NormalW;
    if (length(normal) < 0.01f)
    {
        // 법선이 없다면 강제로 위쪽(Y축)을 바라보게 만들어 수학 에러를 방지합니다.
        normal = float3(0.0f, 1.0f, 0.0f);
    }
    else
    {
        // 법선이 정상적으로 존재할 때만 길이를 1로 맞춥니다.
        normal = normalize(normal);
    }
    //   -------------------------------------------------------------------------------------------------------------  
    float3 lightVec = normalize(-gLightDir);
    float3 viewVec = normalize(gEyePosW - input.PosW);
    
     //   [안전 장치 2] 빛이 닿지 않는 완벽한 역광(그림자) 상태에서도 큐브가 뚜렷하게 보이도록 기본 밝기를 60%로 확 끌어올립니다!  
    float3 ambient = float3(0.6f, 0.6f, 0.6f);
    
    float diffuseFactor = saturate(dot(normal, lightVec)); //    표면 밝기 연산
    float3 diffuse = diffuseFactor * gLightColor.rgb;
    
    float3 halfVec = normalize(lightVec + viewVec); //    광택 연산
    float specFactor = pow(saturate(dot(normal, halfVec)), 64.0f);
    float3 specular = (diffuseFactor > 0.0f) ? (specFactor * gLightColor.rgb * 0.5f) : float3(0.0f, 0.0f, 0.0f);

    // --- [그림자(Shadow) 연산 시작!] ---
    float shadowFactor = 1.0f; //    기본은 그림자 없음(1.0)
    
    input.ShadowPos.xyz /= input.ShadowPos.w; //    UV 좌표 정규화
    
    //   [최적화 수정] 성능을 갉아먹는 if문(분기문)을 완전히 삭제하고 하드웨어와 수학 연산에 맡깁니다!  
    
    // 1. 하드웨어 처리: 범위를 벗어난 X, Y 좌표는 위에서 설정한 '하얀색 테두리(OPAQUE_WHITE)' 덕분에 자동으로 1.0을 반환합니다.
    shadowFactor = gShadowMap.SampleCmpLevelZero(gsSamShadow, input.ShadowPos.xy, input.ShadowPos.z - 0.003f).r;
    
    // 2. 수학적 처리: Z값(거리)이 1.0을 넘어가 빛의 도달 범위를 벗어나면 강제로 그림자를 없앱니다.
    // step(a, x) 함수는 x가 a보다 크면 1.0, 작으면 0.0을 반환합니다. (if문 없이 분기 처리하는 고급 셰이더 스킬!)
    shadowFactor = max(shadowFactor, step(1.0f, input.ShadowPos.z));
    
    shadowFactor = shadowFactor * 0.5f + 0.5f;
    // ------------------------------------------
    
    
     // --- 2.  점광원 (Point Light) 연산 시작!  ---
    // 픽셀의 현재 위치에서 점광원(태양)까지의 3D 화살표(벡터)를 만듭니다.
    float3 pointLightVec = gPointLightPosW - input.PosW;
    // 화살표의 길이 = 빛의 근원지까지의 실제 거리(미터)입니다.
    float d = length(pointLightVec);
    
    float3 finalPointColor = float3(0.0f, 0.0f, 0.0f); // 초기값은 까만색

    // 거리가 소멸 거리(FalloffEnd)보다 가까울 때만 셰이더 연산을 합니다! (최적화) if문일지라도 if문안의 내용이 더 무겁다면 if문을 써야한다.
    if (d < gPointLightFalloffEnd)
    {
        pointLightVec /= d; // 거리를 구했으니 화살표 길이를 1로 정규화(Normalize)합니다.
        
        // 방향광과 똑같이 빛의 각도를 검사해 명암(Diffuse)과 광택(Specular)을 구합니다.
        float pointDiffuseFactor = saturate(dot(normal, pointLightVec));
        float3 pointDiffuse = pointDiffuseFactor * gPointLightColor.rgb;
        
        float3 pointHalfVec = normalize(pointLightVec + viewVec);
        float pointSpecFactor = pow(saturate(dot(normal, pointHalfVec)), 64.0f);
        float3 pointSpecular = (pointDiffuseFactor > 0.0f) ? (pointSpecFactor * gPointLightColor.rgb * 0.5f) : float3(0.0f, 0.0f, 0.0f);
        
        // [감쇠(Attenuation) 수학 공식] 거리가 멀어질수록 빛이 사그라드는 비율을 계산합니다. 
        // 거리가 FalloffStart보다 작으면 1.0(최대), End에 다다르면 0.0으로 부드럽게 감소시킵니다.
        float attenuation = saturate((gPointLightFalloffEnd - d) / (gPointLightFalloffEnd - gPointLightFalloffStart));
        
        // 점광원의 최종 색상에 감쇠 비율을 곱해줍니다.
        finalPointColor = (baseColor.rgb * pointDiffuse + pointSpecular) * attenuation;
    }
    // -------------------------------------------------------------
    
    // 3. 최종 색상 합성! (표면색 * (환경광 + 방향광) + 점광원 + 발광)
    float3 finalColor = (baseColor.rgb * (ambient + diffuse * shadowFactor)) + (specular * shadowFactor);
    finalColor += finalPointColor; //  새로 구한 점광원 불빛 얹기!
    finalColor += input.Emissive;
    
    output.Color = float4(finalColor, 1.0f);
    
    return output;
}