
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
};


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

    float3 pos = float3(g_positions[i_vertex], 0).yzx;
    float2 texCoord = g_positions[i_vertex] + 0.5f;
    float3 normal = float3(0, 1, 1);
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
