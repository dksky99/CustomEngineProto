// ============================================================================
//   [30단계] 무한한 우주 공간을 그리는 스카이박스 셰이더 (sky.hlsl)  
// 외부 텍스처 없이, 오직 수학 노이즈만으로 별과 은하수를 만들어냅니다!
// ============================================================================

// 매 프레임 C++에서 넘겨주는 공통 상수 버퍼(b0)입니다. 카메라 정보가 들어있습니다.
cbuffer cbPass : register(b0)
{
    float4x4 gViewProj; // 3D 월드를 2D 화면으로 변환하는 카메라 행렬입니다.
    float3 gLightDir; // 빛의 방향 (스카이박스에선 안 씀)
    float gTotalTime; // 현재 누적된 게임 시간입니다. (은하수를 흐르게 만듭니다)
    float4 gLightColor; // 빛의 색상 (스카이박스에선 안 씀)
    float3 gEyePosW; // 현재 카메라의 3D 월드 위치입니다.
    float pad; // 16바이트 정렬을 위한 빈칸입니다.
    float4x4 gLightTransform; // 섀도우 맵용 행렬 (스카이박스에선 안 씀)
    float4x4 gLightViewProj; // 태양 시점 행렬 (스카이박스에선 안 씀)
}; 

// 100개 큐브의 위치 배열을 받던 버퍼입니다. (스카이박스는 0번 인덱스 딱 1개만 씁니다)
struct InstanceData
{
    float4x4 World; // 스카이박스 큐브의 월드 행렬입니다.
    float4 BaseColor; // 색상
    float3 Emissive; // 발광
    float pad; // 패딩
};
StructuredBuffer<InstanceData> gInstanceData : register(t1); // t1 레지스터에서 인스턴스 명부를 읽습니다.

// 정점 셰이더(VS)로 들어올 데이터 구조체입니다. (기존 큐브의 데이터를 그대로 씁니다)
struct VSInput
{
    float3 Pos : POSITION; // 꼭짓점의 3D 지역 좌표입니다.
    float4 Color : COLOR; // 색상 (안 씀)
    float3 Normal : NORMAL; // 법선 (안 씀)
    float2 TexC : TEXCOORD; // UV (안 씀)
    
    //  C++에서 'TANGENT(접선)' 데이터를 보내도록 파이프라인 규칙이 바뀌었습니다!
    // 스카이박스는 이 접선 값을 실제로 쓰진 않지만, C++과 서명(Signature)을 완벽히 맞추기 위해 
    // 반드시 입력 변수로 받아주어야만 DX12 초기화 에러(파이프라인 생성 실패)가 나지 않습니다.
    float3 Tangent : TANGENT;
    
    uint instanceID : SV_InstanceID; // 자신이 몇 번째 큐브인지 알려주는 번호표입니다.
};

// 정점 셰이더에서 픽셀 셰이더로 넘겨줄 데이터 구조체입니다.
struct PSInput
{
    float4 PosH : SV_POSITION; // 화면상에 찍힐 최종 2D 픽셀 좌표입니다.
    float3 PosL : POSITION; // 픽셀 셰이더에서 3D 방향을 계산하기 위해 '지역 좌표'를 그대로 넘깁니다.
};

// ============================================================================
// 1. 버텍스 셰이더 (Vertex Shader)
// ============================================================================
PSInput VSMain(VSInput input)
{
    PSInput output; // 출력할 바구니를 만듭니다.
    
   //  C++의 배열(gInstanceData)을 무시하고, 셰이더가 직접 위치를 계산합니다! 
    // DX12에서는 SV_InstanceID가 무조건 0부터 시작하기 때문에, 태양(0번)의 데이터를 훔쳐오는 대참사가 발생했습니다.
    // 아예 배열을 안 쓰고, 카메라 위치(gEyePosW)를 기준으로 500배 크기의 큐브를 직접 만들어냅니다.
    float4 posW = float4(input.Pos * 500.0f + gEyePosW, 1.0f);
    
    // 월드 공간의 좌표를 카메라가 보는 화면 좌표(ViewProj)로 압축합니다.
    output.PosH = mul(posW, gViewProj);
    
   // z와 w가 완전히 똑같으면 부동소수점 오차로 화면에서 날아갑니다.
    // 0.99999f를 곱해주어 가장 뒤에 배경으로 깔리면서도 절대 화면에서 잘리지 않게 보호합니다! 
    output.PosH.z = output.PosH.w * 0.99999f;
    
    // 큐브의 중심(0,0,0)에서 각 꼭짓점으로 향하는 지역 좌표 자체가 곧 '방향 벡터'가 됩니다. 이를 그대로 넘깁니다.
    output.PosL = input.Pos;
    
    return output; // 조립된 데이터를 픽셀 셰이더로 던집니다.
}

// ============================================================================
// 2. 픽셀 셰이더 (Pixel Shader) - 절차적 우주 배경 생성
// ============================================================================
float4 PSMain(PSInput input) : SV_TARGET
{
    // 입력받은 3D 지역 좌표를 정규화하여, 카메라에서 바라보는 완벽한 3D 방향(Direction) 화살표를 만듭니다.
    float3 dir = normalize(input.PosL);
    
    // 1. [은하수 배경 만들기]
    // 방향 벡터의 x, y, z 값과 시간(gTotalTime)을 섞어서 천천히 꿀렁거리는 보라빛 우주 가스를 만듭니다.
    float galaxy1 = sin(dir.x * 5.0f + gTotalTime * 0.1f) * cos(dir.y * 4.0f);
    float galaxy2 = sin(dir.z * 3.0f - gTotalTime * 0.05f) * cos(dir.x * 6.0f);
    float galaxy = (galaxy1 + galaxy2) * 0.5f; // 두 패턴을 섞습니다.
    // 짙은 남색/보라색 톤으로 은하수 색상을 결정합니다.
    float3 skyColor = float3(0.02f, 0.02f, 0.05f) + float3(0.1f, 0.05f, 0.2f) * max(0.0f, galaxy);
    
    // 2. [수많은 별들 뿌리기]
    // 방향 벡터를 내적(dot)하여 엄청나게 큰 숫자를 곱하고 소수점(frac)만 떼어내어 완벽한 난수(Noise)를 생성합니다.
    float noise = frac(sin(dot(dir, float3(12.9898f, 78.233f, 45.164f))) * 43758.5453f);
    
    // 노이즈 값이 0.998보다 큰 픽셀(전체의 0.2%)만 하얗게 칠해서 별처럼 보이게 만듭니다.
    float star = smoothstep(0.998f, 1.0f, noise);
    
    // 3. 은하수 배경에 별들을 더해 최종 우주 색상을 완성합니다!
    float3 finalColor = skyColor + float3(star, star, star);
    
    // 최종 색상을 도화지에 출력합니다.
    return float4(finalColor, 1.0f);
}