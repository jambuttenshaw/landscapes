
#pragma pack_matrix(row_major)

#include <donut/shaders/bindless.h>
#include <donut/shaders/forward_vertex.hlsli>
#include <donut/shaders/gbuffer_cb.h>
#include <donut/shaders/binding_helpers.hlsli>

DECLARE_CBUFFER(GBufferFillConstants, c_GBuffer, GBUFFER_BINDING_VIEW_CONSTANTS, GBUFFER_SPACE_VIEW);



StructuredBuffer<InstanceData> t_Instances : REGISTER_SRV(GBUFFER_BINDING_INSTANCE_BUFFER, GBUFFER_SPACE_INPUT);

static const float2 g_positions[] = {
	float2(-0.5, -0.5),
	float2( 0.5, -0.5),
	float2(-0.5,  0.5),
	float2( 0.5,  0.5)
/*
    // Cut the triangle strip with a degenerate triangle
	float2( 0.5,  0.5),
    float2(1.25f, 0.0f) + float2(-0.5, -0.5),

    float2(1.25f, 0.0f) + float2(-0.5, -0.5),
	float2(1.25f, 0.0f) + float2(0.5, -0.5),
	float2(1.25f, 0.0f) + float2(-0.5, 0.5),
	float2(1.25f, 0.0f) + float2(0.5, 0.5),
*/
};

static const float2 g_terrainDimensions = float2(1, 1);
static const uint2 g_terrainResolution = uint2(2, 2);


void gbuffer_vs(
	in uint i_vertex : SV_VertexID,
	in uint i_instance : SV_InstanceID,
    out float4 o_position : SV_Position,
    out SceneVertex o_vtx,
    out uint o_instance : INSTANCE
)
{
    o_instance = i_instance;

    const InstanceData instance = t_Instances[i_instance];

    //uint2 terrainResolution = uint2(min(g_terrainResolution.x, g_terrainResolution.y), max(g_terrainResolution.x, g_terrainResolution.y));
    //
    //// map vertex ID into a 2D coordinate
    //uint2 vertexID = uint2(i_vertex % terrainResolution.y, i_vertex / terrainResolution.y);
    //// Calculate the position of this vertex on the terrain as if it was a 2D plane being viewed from above
    //float2 terrainUV = float2(vertexID) / float2(terrainResolution - 1);
    //float2 terrainPos = (terrainUV - 0.5f) * g_terrainDimensions;

    float3 pos = float3(g_positions[i_vertex], 0).yzx;

    // The texcoord is the normalized terrainPos
    float2 texCoord = g_positions[i_vertex] + 0.5f;

    float3 normal = float3(0, 1, 0);
    float4 tangent = float4(1, 0, 0, 0);

    o_vtx.pos = mul(instance.transform, float4(pos, 1.0)).xyz;
    o_vtx.texCoord = texCoord;
    o_vtx.normal = mul(instance.transform, float4(normal, 0)).xyz;
    o_vtx.tangent.xyz = mul(instance.transform, float4(tangent.xyz, 0)).xyz;
    o_vtx.tangent.w = tangent.w;
    o_vtx.prevPos = o_vtx.pos;

    float4 worldPos = float4(o_vtx.pos, 1.0);
    o_position = mul(worldPos, c_GBuffer.view.matWorldToClip);
}
