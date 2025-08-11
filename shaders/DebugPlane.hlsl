
#pragma pack_matrix(row_major)

#include <donut/shaders/binding_helpers.hlsli>
#include <donut/shaders/view_cb.h>

#include "DebugTools.h"

DECLARE_CBUFFER(DebugPlaneConstants, c_View, 0, 0);

void debug_plane_vs(
    in uint i_vertex : SV_VertexID,
    in uint i_instance : SV_InstanceID,
    out float4 o_position : SV_Position,
    out float2 o_uv : TEXCOORD0
)
{
	o_uv = float2(i_vertex % 2, i_vertex / 2);

    float3 normal = c_View.PlaneNormal;
    float3 tangent = normal.zxy;
    float3 bitangent = cross(normal, tangent);

    float2 displacement = o_uv * 2.0f - 1.0f;
    float3 worldPos = c_View.PlaneOrigin + c_View.PlaneSize * (tangent * displacement.x + bitangent * displacement.y);

    o_position = mul(float4(worldPos, 1.0), c_View.View.matWorldToClip);
}

void debug_plane_ps(
    in float4 i_position : SV_Position,
    in float2 i_uv : TEXCOORD0,
    in bool i_front : SV_IsFrontFace,
    out float4 o_color : SV_Target0
)
{
	o_color = i_front ? float4(0.4f, 0.4f, 1.0f, 1.0f) : float4(1.0f, 0.4f, 0.4f, 1.0f);
}
