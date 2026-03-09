[[vk::binding(0, 0)]]
cbuffer FrameData
{
    float4x4 viewProjection;
    float3 cameraPos;
    uint lightCount;
};

// DEV WARNING: Binding 1, 0 is reserved for light array

struct PushConstants
{
    float4x4 modelMatrix;
};

[[vk::push_constant]] PushConstants pc;

struct PSInput
{
    float4 pos : SV_Position;
    float3 worldPos : TEXCOORD0;
    float3 normal : NORMAL;
    float3 color : COLOR;
};
