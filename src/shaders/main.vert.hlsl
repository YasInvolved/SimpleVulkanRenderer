struct PushConstants
{
    matrix MVP;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConst;

struct VSInput
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float3 color : COLOR;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    
    float4 pos4 = float4(input.position, 1.0f);
    
    output.pos = mul(pushConst.MVP, pos4);
    output.color = input.color;
    
    return output;
}