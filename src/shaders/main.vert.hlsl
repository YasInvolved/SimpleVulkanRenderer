struct PushConstants
{
    matrix mvp;
};

[[vk::push_constant]] PushConstants pc;

struct VSInput
{
    [[vk::location(0)]] float3 pos : POSITION;
    [[vk::location(1)]] float3 normal : NORMAL;
    [[vk::location(2)]] float3 color : COLOR;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float3 color : COLOR;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    float4 pos4 = float4(input.pos, 1.0f);
    output.pos = mul(pc.mvp, pos4);
    output.normal = input.normal;
    output.color = input.color;
    
    return output;
}
