// CPU로부터 매 프레임마다 변하는 상수(Constant) 데이터를 넘겨받기 위한 버퍼 블록입니다.
// register(b0)는 이 데이터가 파이프라인의 'b0 (상수 버퍼 0번 슬롯)'에 꽂힐 것임을 의미합니다.
cbuffer cbPerObject : register(b0)
{ // 상수 버퍼 블록 시작
    float4x4 gWorldViewProj; // 4x4 크기의 변환 행렬입니다. (World * View * Projection 행렬이 곱해진 최종 결과물)
}; // 상수 버퍼 블록 끝

// 정점 셰이더(Vertex Shader)로 들어올 입력 데이터의 형식을 정의하는 구조체입니다.
struct VSInput
{ // VSInput 구조체 시작
    float3 Pos : POSITION; // 3차원 벡터(X, Y, Z)로 구성된 위치 데이터입니다.
    float4 Color : COLOR; // 4차원 벡터(R, G, B, A)로 구성된 색상 데이터입니다.
}; // VSInput 구조체 끝

// 정점 셰이더가 계산을 마치고 픽셀 셰이더로 넘겨줄 출력 데이터의 형식을 정의하는 구조체입니다.
struct PSInput
{ // PSInput 구조체 시작
    float4 Pos : SV_POSITION; // 화면상에 그려질 최종 2D 좌표 위치입니다. (SV_POSITION)
    float4 Color : COLOR; // 버텍스 셰이더에서 받아온 색상 정보를 그대로 픽셀 셰이더로 전달합니다.
}; // PSInput 구조체 끝

// 버텍스 셰이더의 메인 함수입니다. (각각의 꼭짓점마다 한 번씩 실행됩니다)
PSInput VSMain(VSInput input)
{ // VSMain 함수 시작
    PSInput output; // 다음 단계로 넘겨줄 출력용 빈 구조체를 하나 생성합니다.
    
    // 들어온 3D 지역 좌표(Local Position)에 w값 1.0을 추가하여 4D 벡터로 만듭니다.
    float4 posW = float4(input.Pos, 1.0f);
    
    // 이 정점의 위치에 CPU에서 넘겨준 3D 변환 행렬(gWorldViewProj)을 곱(mul)합니다!
    // 이 한 줄의 연산이 2D였던 폴리곤을 원근감이 적용된 3D 공간의 위치로 튕겨내 줍니다.
    output.Pos = mul(posW, gWorldViewProj);
    
    // 정점이 가지고 있던 색상 데이터는 변형 없이 그대로 출력 구조체에 복사합니다.
    output.Color = input.Color;
    
    return output; // 위치와 색상이 채워진 결과물을 픽셀 셰이더로 반환하여 넘깁니다.
} // VSMain 함수 끝

// 픽셀 셰이더의 메인 함수입니다. (삼각형 내부를 채우는 모든 픽셀마다 실행됩니다)
float4 PSMain(PSInput input) : SV_TARGET
{ // PSMain 함수 시작
    // 버텍스 셰이더에서 넘겨준 색상(Color) 값을 그대로 최종 픽셀의 색상으로 결정하여 반환합니다.
    return input.Color;
} // PSMain 함수 끝