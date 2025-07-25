
#pragma pack_matrix(row_major)

#include <donut/shaders/binding_helpers.hlsli>
#include <donut/shaders/gbuffer.hlsli>
#include <donut/shaders/surface.hlsli>


struct GBufferVisualizationConstants
{
    uint2 Dims;
};
DECLARE_PUSH_CONSTANTS(GBufferVisualizationConstants, g_GBuffer, 0, 0);

Texture2D t_GBufferDepth : REGISTER_SRV(0, 0);
Texture2D t_GBuffer0 : REGISTER_SRV(1, 0);
Texture2D t_GBuffer1 : REGISTER_SRV(2, 0);
Texture2D t_GBuffer2 : REGISTER_SRV(3, 0);
Texture2D t_GBuffer3 : REGISTER_SRV(4, 0);

RWTexture2D<float4> u_Output : REGISTER_UAV(0, 0);


MaterialSample GetMaterialData(uint2 pixelPos)
{
    float4 gbufferChannels[4];
    gbufferChannels[0] = t_GBuffer0[pixelPos];
    gbufferChannels[1] = t_GBuffer1[pixelPos];
    gbufferChannels[2] = t_GBuffer2[pixelPos];
    gbufferChannels[3] = t_GBuffer3[pixelPos];
    return DecodeGBuffer(gbufferChannels);
}


[numthreads(16, 16, 1)]
void visualize_unlit_cs(uint3 DTid : SV_DispatchThreadID)
{
    if (any(DTid.xy >= g_GBuffer.Dims))
    {
        return;
    }

    MaterialSample data = GetMaterialData(DTid.xy);
    float3 unlitColour = data.diffuseAlbedo;

    u_Output[DTid.xy] = float4(unlitColour, 1.0f);
}


[numthreads(16, 16, 1)]
void visualize_normals_cs(uint3 DTid : SV_DispatchThreadID)
{
    if (any(DTid.xy >= g_GBuffer.Dims))
	{
        return;
    }

    MaterialSample data = GetMaterialData(DTid.xy);
    float3 normal = data.shadingNormal;
    
    u_Output[DTid.xy] = float4(normal, 1.0f);
}
