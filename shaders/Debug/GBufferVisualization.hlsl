
#pragma pack_matrix(row_major)

#include <donut/shaders/binding_helpers.hlsli>

struct GBufferVisualizationConstants
{
    uint2 Dims;
};
DECLARE_PUSH_CONSTANTS(GBufferVisualizationConstants, g_GBuffer, 0, 0);

Texture2D t_GBufferNormals : REGISTER_SRV(0, 0);
SamplerState s_Sampler : REGISTER_SAMPLER(0, 0);

RWTexture2D<float4> u_Output : REGISTER_UAV(0, 0);


[numthreads(16, 16, 1)]
void visualize_normals_cs(uint3 DTid : SV_DispatchThreadID)
{
    if (any(DTid.xy >= g_GBuffer.Dims))
	{
        return;
    }

    float2 uv = (float2) DTid.xy / (float2) (g_GBuffer.Dims - 1);
    float3 normal = t_GBufferNormals.SampleLevel(s_Sampler, uv, 0).rgb;

    //normal = all(normal == 0.0f) ? normal : normal * 0.5f + 0.5f;

    u_Output[DTid.xy] = float4(normal, 1.0f);
}
