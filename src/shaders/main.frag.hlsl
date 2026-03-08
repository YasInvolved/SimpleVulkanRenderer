// MIT License
// 
// Copyright(c) 2026 Jakub Bączyk
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "include/constants.hlsl"
#include "include/shared.hlsl"

// hardcoded material test
static const float3 albedo = float3(1.0f, 0.86f, 0.57f);
static const float metallic = 1.0f;
static const float roughness = 0.2f;
static const float ao = 1.0f;
static const float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
static const uint lightCount = 2u;
static const float3 lightPositions[] = { float3(2.0f, 4.0f,-6.0f), float3(-2.0f, 4.0f, -6.0f) };
static const float3 lightColors[] = { float3(1.0f, 0.8f, 0.6f), float3(0.6f, 0.8f, 1.0f) };
static const float lightIntensities[] = { 150.0f, 150.0f };

float distGGX(float3 N, float3 H, float roughness)
{
    const float a2 = pow(roughness, 4.0f);
    const float NdotH = max(dot(N, H), 0.0f);
    const float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return a2 / denom;
}

float schlickGGX(float NdotV, float roughness)
{
    const float r = roughness + 1.0f;
    const float k = (r * r) / 8.0f;

    const float denom = NdotV * (1.0f - k) + k;

    return NdotV / denom;
}

float geomSmith(float3 N, float3 V, float3 L, float roughness)
{
    const float NdotV = max(dot(N, V), 0.0f);
    const float NdotL = max(dot(N, L), 0.0f);

    const float ggx2 = schlickGGX(NdotV, roughness);
    const float ggx1 = schlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

float4 PSMain(PSInput input) : SV_Target
{
    float3 N = normalize(input.normal);
    float3 V = normalize(pc.cameraPos - input.worldPos);

    float3 Lo = float3(0.0f, 0.0f, 0.0f);
    
    for (uint i = 0; i < lightCount; i++)
    {
        const float3 lightPos = lightPositions[i];
        const float3 lightColor = lightColors[i];
        const float intensity = lightIntensities[i];

        const float3 L = normalize(lightPos - input.worldPos);
        const float3 H = normalize(V + L);

        const float distance = length(lightPos - input.worldPos);
        const float attenuation = 1.0f / (distance * distance);
        const float3 radiance = lightColor * intensity * attenuation;

        const float NDF = distGGX(N, H, roughness);
        const float G = geomSmith(N, V, L, roughness);
        const float3 F = fresnelSchlick(max(dot(H, V), 0.0f), F0);

        const float3 numerator = NDF * G * F;
        const float denom = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.0001;
        const float3 specular = numerator / denom;

        float3 kD = float3(1.0f, 1.0f, 1.0f) - F;
        kD *= 1.0f - metallic;

        const float NdotL = max(dot(N, L), 0.0f);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    const float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo * ao;
    float3 color = ambient + Lo;
    color = color / (color + float3(1.0f, 1.0f, 1.0f));
  
    return float4(color, 1.0f);
}
