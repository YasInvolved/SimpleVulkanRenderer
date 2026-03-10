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

#include "include/shared.hlsl"

struct VSInput
{
    [[vk::location(0)]] float3 pos : POSITION;
    [[vk::location(1)]] float3 normal : NORMAL;
    [[vk::location(2)]] float3 color : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 pos4 = float4(input.pos, 1.0f);
    const float4x4 mvp = mul(viewProjection, pc.modelMatrix);
    
    output.pos = mul(mvp, pos4);

    const float4 worldPos = mul(pc.modelMatrix, float4(input.pos, 1.0f));
    output.worldPos = worldPos.xyz;
    
    output.normal = normalize(mul((float3x3)pc.modelMatrix, input.normal));
    output.color = input.color;
    
    return output;
}
