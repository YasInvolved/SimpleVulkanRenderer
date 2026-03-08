struct PushConstants
{
    float4x4 vp;
    float4x4 model;
    float3 cameraPos;
};

[[vk::push_constant]] PushConstants pc;

struct PSInput
{
    float4 pos : SV_Position;
    float3 worldPos : TEXCOORD0;
    float3 normal : NORMAL;
    float3 color : COLOR;
};
