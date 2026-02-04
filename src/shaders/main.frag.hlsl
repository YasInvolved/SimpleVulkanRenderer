struct PSInput
{
    float4 pos : SV_Position;
    float3 color : COLOR;
};

float4 PSMain(PSInput input) : SV_Target0
{
    return float4(input.color, 1.0f);
}