struct PSInput
{
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float3 color : COLOR;
};

float4 PSMain(PSInput input) : SV_Target
{
    return float4(input.color, 1.0f);
}
