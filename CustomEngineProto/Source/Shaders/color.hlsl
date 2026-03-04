// CPU로부터 매 프레임마다 변하는 상수(Constant) 데이터를 넘겨받기 위한 버퍼 블록입니다.
// register(b0)는 이 데이터가 파이프라인의 'b0 (상수 버퍼 0번 슬롯)'에 꽂힐 것임을 의미합니다.
cbuffer cbPerObject : register(b0)
{ // 상수 버퍼 블록 시작
    float4x4 gWorldViewProj; // 4x4 크기의 변환 행렬입니다. (World * View * Projection 행렬이 곱해진 최종 화면 좌표용)
    float4x4 gWorld; // 4x4 크기의 월드 행렬입니다. (법선 벡터를 3D 월드 공간으로 변환하기 위해 추가되었습니다!)
    float3 gLightDir; // 빛이 쏟아지는 방향 벡터입니다. (예: 위에서 아래로)
    float pad; // 16바이트 정렬(Alignment)을 맞추기 위한 빈칸(패딩)입니다.
    float4 gLightColor; // 빛의 색상입니다. (R, G, B, A)
}; // 상수 버퍼 블록 끝

// 정점 셰이더(Vertex Shader)로 들어올 입력 데이터의 형식을 정의하는 구조체입니다.
// 나중에 C++ 코드에서 버텍스 버퍼(Vertex Buffer)를 만들 때 이 형식과 정확히 일치시켜야 합니다.
struct VSInput
{ // VSInput 구조체 시작
    float3 Pos : POSITION; // 3차원 벡터(X, Y, Z)로 구성된 위치(Position) 데이터입니다. 시맨틱(Semantic) 이름은 POSITION입니다.
    float4 Color : COLOR; // 4차원 벡터(R, G, B, A)로 구성된 색상(Color) 데이터입니다. 시맨틱 이름은 COLOR입니다.
    float3 Normal : NORMAL; // --- 새롭게 추가됨: 이 꼭짓점이 바라보는 방향(법선 벡터)입니다. ---
}; // VSInput 구조체 끝

// 정점 셰이더가 계산을 마치고 픽셀 셰이더(Pixel Shader)로 넘겨줄 출력 데이터의 형식을 정의하는 구조체입니다.
struct PSInput
{ // PSInput 구조체 시작
    float4 Pos : SV_POSITION; // 화면상에 그려질 최종 2D 좌표 위치입니다. SV_POSITION은 시스템(SV)이 예약한 아주 중요한 키워드입니다.
    float4 Color : COLOR; // 버텍스 셰이더에서 받아온 색상 정보를 그대로 픽셀 셰이더로 전달하기 위한 변수입니다.
    float3 NormalW : NORMAL; // --- 새롭게 추가됨: 월드 공간(World Space)으로 변환된 법선 벡터입니다. ---
}; // PSInput 구조체 끝

// 1. 버텍스 셰이더의 메인 함수입니다. (각각의 꼭짓점마다 한 번씩 실행됩니다)
PSInput VSMain(VSInput input)
{ // VSMain 함수 시작
    PSInput output; // 다음 단계(픽셀 셰이더)로 넘겨줄 출력용 빈 구조체를 하나 생성합니다.
    
   
    
    // 1. 위치 변환: 지역 좌표에 1.0을 더해 4D로 만들고 최종 행렬을 곱해 화면 좌표로 변환합니다.
     // 들어온 3D 지역 좌표(Local Position)에 w값 1.0을 추가하여 4D 벡터로 만듭니다.
    float4 posW = float4(input.Pos, 1.0f);
    // 이 정점의 위치에 CPU에서 넘겨준 3D 변환 행렬(gWorldViewProj)을 곱(mul)합니다!
    // 이 한 줄의 연산이 2D였던 폴리곤을 원근감이 적용된 3D 공간의 위치로 튕겨내 줍니다.
    output.Pos = mul(posW, gWorldViewProj);
    
     // 2. 법선 변환: 지역 좌표계에 있던 법선 화살표를 월드 행렬(gWorld)을 곱해 월드 공간의 화살표로 변환합니다.
    output.NormalW = mul(input.Normal, (float3x3) gWorld);
    // 정점이 가지고 있던 색상 데이터는 변형 없이 그대로 출력 구조체에 복사합니다.
    output.Color = input.Color;
    
    return output; // 위치와 색상이 채워진 결과물을 픽셀 셰이더로 반환하여 넘깁니다.
} // VSMain 함수 끝

// 2. 픽셀 셰이더의 메인 함수입니다. (삼각형 내부를 채우는 모든 픽셀마다 한 번씩 엄청나게 많이 실행됩니다)
// SV_TARGET 키워드는 반환되는 색상 값이 우리가 만든 렌더 타겟 뷰(RTV, 현재 화면)에 칠해져야 함을 의미합니다.
float4 PSMain(PSInput input) : SV_TARGET
{ // PSMain 함수 시작
     // 1. 법선 정규화: 보간되는 과정에서 길이가 변했을 수 있으므로, 화살표의 길이를 다시 1로(Normalize) 맞춰줍니다.
    float3 n = normalize(input.NormalW);
    
    // 2. 빛의 방향 반전: 빛이 표면으로 '들어오는' 방향을, 표면에서 빛을 향해 '뻗어나가는' 방향으로 뒤집어줍니다. (-gLightDir)
    float3 l = normalize(-gLightDir);
    
    // 3. 내적(Dot Product) 계산: 법선(n)과 빛 방향(l)의 각도를 내적하여 얼만큼 빛을 정면으로 받는지(0.0 ~ 1.0) 계산합니다.
    // 각도가 90도를 넘어가면 음수가 되므로, max 함수를 써서 최소 0.0(그림자)으로 컷오프 해줍니다.
    float ndotl = max(dot(n, l), 0.0f);
    
    // 4. 환경광(Ambient): 빛을 직접 받지 못하는 곳도 완전히 새까맣게 타버리지 않도록 최소한의 밝기(20%)를 줍니다.
    float3 ambient = float3(0.2f, 0.2f, 0.2f);
    
    // 5. 난반사(Diffuse): 빛의 색상에 방금 구한 빛의 세기(ndotl)를 곱해줍니다.
    float3 diffuse = ndotl * gLightColor.rgb;
    
    // 6. 최종 색상 계산: 물체 본래의 색상(input.Color)에 (환경광 + 난반사)를 곱해 최종 픽셀 색상을 완성합니다!
    float3 finalColor = input.Color.rgb * (ambient + diffuse);
    
    
    
    // 버텍스 셰이더에서 넘겨준 색상(Color) 값을 그대로 최종 픽셀의 색상으로 결정하여 반환합니다.
    // (이 과정에서 삼각형의 세 꼭짓점 색상이 다를 경우, 중간 픽셀들은 자동으로 자연스럽게 섞인(Interpolated) 색상이 됩니다)
    return float4(finalColor, input.Color.a); // 화면에 색상 출력

} // PSMain 함수 끝