
#pragma pack_matrix(row_major)

#include <donut/shaders/bindless.h>
#include <donut/shaders/forward_vertex.hlsli>
#include <donut/shaders/gbuffer_cb.h>
#include <donut/shaders/binding_helpers.hlsli>

#include "TerrainShaders.h"

#define CBT_HEAP_BUFFER_BINDING REGISTER_SRV(GBUFFER_BINDING_TERRAIN_CBT, GBUFFER_SPACE_TERRAIN)
#include "ConcurrentBinaryTree.hlsl"
#include "LongestEdgeBisection.hlsl"

DECLARE_CBUFFER(GBufferFillConstants, c_GBuffer, GBUFFER_BINDING_VIEW_CONSTANTS, GBUFFER_SPACE_VIEW);
DECLARE_PUSH_CONSTANTS(TerrainPushConstants, g_Push, GBUFFER_BINDING_PUSH_CONSTANTS, GBUFFER_SPACE_INPUT);

StructuredBuffer<InstanceData> t_Instances : REGISTER_SRV(GBUFFER_BINDING_INSTANCE_BUFFER, GBUFFER_SPACE_INPUT);

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
    o_instance = g_Push.startInstanceLocation;

    // Terrain rendering uses instancing differently than other opaque geometry
	// Each triangle is a different instance, and the instance ID is used to determine which node in the CBT it is
    const InstanceData instance = t_Instances[g_Push.startInstanceLocation];

    // Use LEB to find vertex position
    cbt_Node node = cbt_DecodeNode(i_instance);

    float3x2 posMatrix = float3x2(float2(0, 1),
								  float2(0, 0),
								  float2(1, 0));
    posMatrix = leb_DecodeAttributeArray(node, posMatrix);

    float2 texCoord = float2(posMatrix[i_vertex][0], posMatrix[i_vertex][1]);

    float2 pos = (texCoord - 0.5f) * c_Terrain.TerrainExtentsAndInvExtents.xy;
    float3 localPos = float3(pos.x, GetTerrainHeight(texCoord), pos.y);

    float3 worldPos = mul(instance.transform, float4(localPos, 1.0)).xyz;

    float3 normal = float3(0, 1, 0);
    float3 tangent = float3(1, 0, 0);

    o_vtx.pos = worldPos;
    o_vtx.texCoord = texCoord;
    o_vtx.normal = mul(instance.transform, float4(normal, 0)).xyz;
    o_vtx.tangent.xyz = mul(instance.transform, float4(tangent, 0)).xyz;
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
