
#pragma pack_matrix(row_major)

#include <donut/shaders/bindless.h>
#include <donut/shaders/forward_vertex.hlsli>
#include <donut/shaders/gbuffer_cb.h>
#include <donut/shaders/binding_helpers.hlsli>

DECLARE_CBUFFER(GBufferFillConstants, c_GBuffer, GBUFFER_BINDING_VIEW_CONSTANTS, GBUFFER_SPACE_VIEW);

StructuredBuffer<InstanceData> t_Instances : REGISTER_SRV(GBUFFER_BINDING_INSTANCE_BUFFER, GBUFFER_SPACE_INPUT);
StructuredBuffer<float3> t_Positions : REGISTER_SRV(GBUFFER_BINDING_VERTEX_BUFFER, GBUFFER_SPACE_INPUT);


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

    float3 pos = t_Positions[i_vertex];
    // The texcoord is the normalized terrainPos
    float2 texCoord = t_Positions[i_vertex].xz;
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


void gbuffer_ps(
    in float4 i_position : SV_Position,
	in SceneVertex i_vtx,
    out float4 o_channel0 : SV_Target0,
    out float4 o_channel1 : SV_Target1,
    out float4 o_channel2 : SV_Target2,
    out float4 o_channel3 : SV_Target3
#if MOTION_VECTORS
    , out float3 o_motion : SV_Target4
#endif
)
{
    float3 diffuseAlbedo = 1.0f;
    float opacity = 1.0f;
    float3 specularF0 = 0.0f;
    float occlusion = 0.0f;
    float3 shadingNormal = i_vtx.normal;
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
