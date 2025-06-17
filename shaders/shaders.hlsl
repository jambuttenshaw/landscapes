
#pragma pack_matrix(row_major)

#include <donut/shaders/view_cb.h>

ConstantBuffer<PlanarViewConstants> g_View : register(b0);

static const float2 g_positions[] = {
	float2(-0.5, -0.5),
	float2(-0.5,  0.5),
	float2( 0.5, -0.5),
	float2( 0.5,  0.5)
};

struct VSToPS
{
    float4 pos : SV_Position;
	float3 color : COLOR;
};

VSToPS main_vs(
	uint i_vertexId : SV_VertexID
)
{
    float4 worldPos = float4(g_positions[i_vertexId], 0, 1);
    float4 clipPos = mul(worldPos, g_View.matWorldToClip);

    VSToPS vsOut;
	vsOut.pos = clipPos;
    vsOut.color = float3(g_positions[i_vertexId] + 0.5f, 0.0f);
    return vsOut;
}

float4 main_ps(
	VSToPS psIn
) : SV_Target0
{
    return float4(psIn.color, 1);
}
