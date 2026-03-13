// 상수 버퍼(cbuffer)를 역할에 맞게 2개로 완벽히 분리합니다! 

// 1. 매 프레임 단 1번만 바뀌는 '공통 데이터 (Pass Constants)' - b0 레지스터
cbuffer cbPass : register(b0)
{
    float4x4 gViewProj; // 카메라의 View * Projection 행렬입니다.
    float3 gLightDir; // 공통 빛 방향입니다.
    // [ ] 시간과 카메라 위치를 받을 변수를 셰이더에도 똑같이 추가합니다! 
    float gTotalTime; // 텍스처를 움직이게 만들 '현재 시간'입니다.
    float4 gLightColor; // 공통 빛 색상입니다.
    float3 gEyePosW; // 스페큘러(반사광)를 계산하기 위한 '카메라의 현재 월드 위치'입니다.
    float pad; // 16바이트 정렬을 맞추기 위한 빈칸입니다.
}; 

//[ ] 상수 버퍼(cbuffer)를 버리고, 거대한 데이터 배열(StructuredBuffer)을 사용합니다! 
// 큐브 하나가 가질 데이터를 구조체로 정의합니다.
struct InstanceData
{
    float4x4 World;
};
// 100개의 큐브 위치 데이터가 담긴 배열(명부)을 통째로 t1 레지스터로 받습니다!
StructuredBuffer<InstanceData> gInstanceData : register(t1);

// 텍스처 이미지 데이터(t0)와, 텍스처를 읽어올 방식(샘플러, s0)을 선언합니다.
Texture2D gDiffuseMap : register(t0);
SamplerState gsSamPointWrap : register(s0);


// 정점 셰이더(Vertex Shader)로 들어올 입력 데이터의 형식을 정의하는 구조체입니다.
// 나중에 C++ 코드에서 버텍스 버퍼(Vertex Buffer)를 만들 때 이 형식과 정확히 일치시켜야 합니다.
struct VSInput
{ // VSInput 구조체 시작
    float3 Pos : POSITION; // 3차원 벡터(X, Y, Z)로 구성된 위치(Position) 데이터입니다. 시맨틱(Semantic) 이름은 POSITION입니다.
    float4 Color : COLOR; // 4차원 벡터(R, G, B, A)로 구성된 색상(Color) 데이터입니다. 시맨틱 이름은 COLOR입니다.
    float3 Normal : NORMAL; // --- 새롭게 추가됨: 이 꼭짓점이 바라보는 방향(법선 벡터)입니다. ---
     // C++에서 넘겨줄 텍스처 UV 좌표(2D)를 추가합니다.
    float2 TexC : TEXCOORD;
    
     //  [ ] GPU가 "지금 내가 몇 번째 큐브를 그리고 있지?"를 알 수 있게 해주는 마법의 시스템 변수입니다.
    uint instanceID : SV_InstanceID;
}; // VSInput 구조체 끝

// 정점 셰이더가 계산을 마치고 픽셀 셰이더(Pixel Shader)로 넘겨줄 출력 데이터의 형식을 정의하는 구조체입니다.
struct PSInput
{ // PSInput 구조체 시작
    float4 Pos : SV_POSITION; // 화면상에 그려질 최종 2D 좌표 위치입니다. SV_POSITION은 시스템(SV)이 예약한 아주 중요한 키워드입니다.
     //  [ ] 픽셀 셰이더에서 반사 각도를 계산하기 위해, 점의 '월드 공간 위치'를 하나 더 넘겨줍니다. 
    float3 PosW : POSITION;
    float4 Color : COLOR; // 버텍스 셰이더에서 받아온 색상 정보를 그대로 픽셀 셰이더로 전달하기 위한 변수입니다.
    float3 NormalW : NORMAL; // --- 새롭게 추가됨: 월드 공간(World Space)으로 변환된 법선 벡터입니다. ---
    
    float2 TexC : TEXCOORD; // 버텍스 셰이더에서 픽셀 셰이더로 넘겨줄 UV 좌표입니다.
}; // PSInput 구조체 끝

// [ ] 포스트 프로세싱을 위한 픽셀 셰이더 출력 구조체를 만듭니다! 
// 지금은 렌더 타겟이 1개(모니터 화면)뿐이지만, 나중에 렌더 타겟을 여러 개(MRT)로 늘리거나
// 포스트 프로세싱을 할 때 이 구조체가 아주 요긴하게 쓰입니다.
struct PSOutput
{
    float4 Color : SV_TARGET0; // 첫 번째 렌더 타겟(우리의 메인 도화지)에 출력될 색상입니다.
};


// 1. 버텍스 셰이더의 메인 함수입니다. (각각의 꼭짓점마다 한 번씩 실행됩니다)
PSInput VSMain(VSInput input)
{ // VSMain 함수 시작
    PSInput output; // 다음 단계(픽셀 셰이더)로 넘겨줄 출력용 빈 구조체를 하나 생성합니다.
    
    //   배열(gInstanceData)에서 자신의 인스턴스 번호(instanceID)에 맞는 월드 행렬을 쏙 빼옵니다!
    float4x4 worldMat = gInstanceData[input.instanceID].World;
    
    // 1. 위치 변환: 지역 좌표에 1.0을 더해 4D로 만들고 최종 행렬을 곱해 화면 좌표로 변환합니다.
     // 들어온 3D 지역 좌표(Local Position)에 w값 1.0을 추가하여 4D 벡터로 만듭니다.
    float4 posW = float4(input.Pos, 1.0f);
    
    
     //  누락되었던 핵심 곱셈! 먼저 점들을 월드 행렬을 이용해 3D 공간 각자의 위치(격자 배열)로 보냅니다! 
    posW = mul(posW, worldMat);
    //  [ ] 픽셀 셰이더에게 내 진짜 월드 위치(X,Y,Z)가 어디인지 알려줍니다. 
    output.PosW = posW.xyz;
    // 이 정점의 위치에 CPU에서 넘겨준 3D 변환 행렬(gWorldViewProj)을 곱(mul)합니다!
    // 이 한 줄의 연산이 2D였던 폴리곤을 원근감이 적용된 3D 공간의 위치로 튕겨내 줍니다.
    output.Pos = mul(posW, gViewProj);
    
    
    // 법선 벡터도 현재 큐브의 회전(World)을 반영해 변환합니다.
    output.NormalW = mul(input.Normal, (float3x3) worldMat);
    // 정점이 가지고 있던 색상 데이터는 변형 없이 그대로 출력 구조체에 복사합니다.
    output.Color = input.Color;
    
  //  [ ] 마법의 UV 스크롤링! (텍스처 애니메이션) 
    // 원래의 UV 좌표에 시간을 더해주면, 텍스처 이미지가 대각선으로 영원히 흘러가는 것처럼 보입니다!
    output.TexC = input.TexC + float2(gTotalTime * 0.1f, gTotalTime * 0.1f);
    
    return output; // 위치와 색상이 채워진 결과물을 픽셀 셰이더로 반환하여 넘깁니다.
} // VSMain 함수 끝

// 2. 픽셀 셰이더의 메인 함수입니다. (삼각형 내부를 채우는 모든 픽셀마다 한 번씩 엄청나게 많이 실행됩니다)
// SV_TARGET 키워드는 반환되는 색상 값이 우리가 만든 렌더 타겟 뷰(RTV, 현재 화면)에 칠해져야 함을 의미합니다.
//  [ ] 픽셀 셰이더의 반환 타입을 구조체(PSOutput)로 변경합니다. 
PSOutput PSMain(PSInput input) : SV_TARGET
{ // PSMain 함수 시작
    PSOutput output; // 결과를 담을 빈 출력 구조체를 만듭니다.

    float4 texColor = gDiffuseMap.Sample(gsSamPointWrap, input.TexC);

    // 알파 테스팅 코드를 삭제합니다! 
    // 이제는 픽셀을 버리지 않고 뒤의 색상과 반투명하게 섞을 것이기 때문에 clip() 함수가 필요 없습니다.
    // clip(texColor.a - 0.1f); // <-- 11단계에서 제거되었음

    // [최적화 & 개선] 빛 계산 공식을 조금 더 게임 엔진스럽게 다듬습니다! 
    float3 normal = normalize(input.NormalW);
    float3 lightVec = normalize(-gLightDir); // 빛이 뻗어나가는 방향
    float3 viewVec = normalize(gEyePosW - input.PosW); // 내 눈이 바라보는 방향
    
    // 1. 환경광 (Ambient) - 기본 밝기
    float3 ambient = gLightColor.rgb * 0.2f;
    
    // 2. 난반사광 (Diffuse) - 표면의 거친 정도 (Lambert)
    // saturate는 0~1 사이로 값을 제한하는 함수입니다. max(..., 0.0f)보다 빠릅니다.
    float diffuseFactor = saturate(dot(normal, lightVec));
    float3 diffuse = diffuseFactor * gLightColor.rgb;
    
    // 3. 정반사광 (Specular) - 표면의 매끄러운 정도 (Blinn-Phong)
    // reflect를 쓰는 대신 하프 벡터(Half Vector)를 사용하는 방식이 더 가볍고 널리 쓰입니다.
    float3 halfVec = normalize(lightVec + viewVec); // 빛과 눈의 중간 벡터
    float specFactor = pow(saturate(dot(normal, halfVec)), 64.0f); // 64: 광택의 선명도
    
    // 빛이 아예 안 닿는 곳(diffuseFactor == 0)에는 광택(Specular)도 생기면 안 됩니다!
    float3 specular = (diffuseFactor > 0.0f) ? (specFactor * gLightColor.rgb * 0.5f) : float3(0.0f, 0.0f, 0.0f);

    // 최종 색상 조합!
    float3 finalColor = (texColor.rgb * (ambient + diffuse)) + specular;
    
     // 계산된 최종 색상을 출력 구조체에 담습니다. (알파 값은 텍스처의 알파 유지)
    output.Color = float4(finalColor, texColor.a);
    
   //  [  5] 투명도 값을 포함하여 렌더 타겟에 반환합니다! 
    // 이 픽셀의 투명도(a)를 파이프라인(PSO)의 블렌더에게 넘겨주어 배경과 섞이게 만듭니다.
    return output;

} // PSMain 함수 끝