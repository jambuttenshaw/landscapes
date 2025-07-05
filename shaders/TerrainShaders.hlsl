
#pragma pack_matrix(row_major)

#include <donut/shaders/bindless.h>
#include <donut/shaders/forward_vertex.hlsli>
#include <donut/shaders/gbuffer_cb.h>
#include <donut/shaders/binding_helpers.hlsli>

#include "TerrainShaders.h"


DECLARE_CBUFFER(GBufferFillConstants, c_GBuffer, GBUFFER_BINDING_VIEW_CONSTANTS, GBUFFER_SPACE_VIEW);

DECLARE_PUSH_CONSTANTS(TerrainPushConstants, g_Push, GBUFFER_BINDING_PUSH_CONSTANTS, GBUFFER_SPACE_INPUT);

StructuredBuffer<InstanceData> t_Instances : REGISTER_SRV(GBUFFER_BINDING_INSTANCE_BUFFER, GBUFFER_SPACE_INPUT);
StructuredBuffer<float3> t_Positions : REGISTER_SRV(GBUFFER_BINDING_VERTEX_BUFFER, GBUFFER_SPACE_INPUT);

DECLARE_CBUFFER(TerrainConstants, c_Terrain, GBUFFER_BINDING_TERRAIN_CONSTANTS, GBUFFER_SPACE_TERRAIN);
Texture2D<float> t_HeightmapTexture : REGISTER_SRV(GBUFFER_BINDING_TERRAIN_HEIGHTMAP_TEXTURE, GBUFFER_SPACE_TERRAIN);

SamplerState s_HeightmapSampler : REGISTER_SAMPLER(GBUFFER_BINDING_TERRAIN_HEIGHTMAP_SAMPLER, GBUFFER_SPACE_VIEW);

#include "TerrainHelpers.hlsli"


void gbuffer_vs(
	in uint i_vertex : SV_VertexID,
	in uint i_instance : SV_InstanceID,
    out float4 o_position : SV_Position,
    out SceneVertex o_vtx,
    out uint o_instance : INSTANCE
)
{
    o_instance = i_instance;

    i_instance += g_Push.startInstanceLocation;

    const InstanceData instance = t_Instances[i_instance];

    float3 pos = t_Positions[i_vertex];
    float3 worldPos = mul(instance.transform, float4(pos, 1.0)).xyz;
    float2 texCoord = (worldPos.xz * c_Terrain.TerrainExtentsAndInvExtents.zw) + 0.5f;

    worldPos.y += GetTerrainHeight(texCoord);

    float3 normal = float3(0, 1, 0);
    float3 tangent = float3(1, 0, 0);

    o_vtx.pos = worldPos;
    o_vtx.texCoord = texCoord;
    o_vtx.normal = mul(instance.transform, float4(normal, 0)).xyz;
    o_vtx.tangent.xyz = mul(instance.transform, float4(tangent, 0)).xyz;
    o_vtx.normal = normal;
    o_vtx.tangent.xyz = tangent;
    o_vtx.tangent.w = 0.0f;
    o_vtx.prevPos = o_vtx.pos;

    o_position = mul(float4(worldPos, 1.0), c_GBuffer.view.matWorldToClip);
}


void gbuffer_ps(
    in float4 i_position : SV_Position,
	in SceneVertex i_vtx,
    out float4 o_channel0 : SV_Target0,
    out float4 o_channel1 : SV_Target1,
    out float4 o_channel2 : SV_Target2,
    out float4 o_channel3 : SV_Target3
)
{
    float3 diffuseAlbedo = float3(1,1,1);
    float opacity = 1.0f;
    float3 specularF0 = 0.0f;
    float occlusion = 0.0f;
    float3 shadingNormal = GetTerrainNormal(i_vtx.texCoord);
    float roughness = 1.0f;
    float3 emissiveColor = 0.0f;

    o_channel0.xyz = diffuseAlbedo;
    o_channel0.w = opacity;
    o_channel1.xyz = specularF0;
    o_channel1.w = occlusion;
    o_channel2.xyz = shadingNormal;
    o_channel2.w = roughness;
    o_channel3.xyz = emissiveColor;
    o_channel3.w = 0;
}
