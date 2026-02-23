static const float PI = 3.14159265f;

struct PSInput
{
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float3 color : COLOR;
};

float distGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    denom = PI * denom * denom;
    return a2 / denom;
}

float geomSchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    float denom = NdotV * (1.0f - k) + k;
    return NdotV / denom;
}

float geomSmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = geomSchlickGGX(NdotV, roughness);
    float ggx1 = geomSchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 f0)
{
    return f0 + (float3(1.0f, 1.0f, 1.0f) - f0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

float4 PSMain(PSInput input) : SV_Target
{
    return float4(input.color, 1.0f);
}
