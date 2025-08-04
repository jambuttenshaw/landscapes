
#pragma pack_matrix(row_major)

#include <donut/shaders/binding_helpers.hlsli>
#include <donut/shaders/bindless.h>
#include "TerrainShaders.h"

#define CBT_FLAG_WRITE

#define CBT_HEAP_BUFFER_BINDING REGISTER_UAV(TESSELLATION_BINDING_CBT, TESSELLATION_SPACE_TERRAIN)
#include "ConcurrentBinaryTree.hlsl"
#include "LongestEdgeBisection.hlsl"

DECLARE_CBUFFER(SubdivisionConstants, c_Subdivision, TESSELLATION_BINDING_SUBDIVISION_CONSTANTS, TESSELLATION_SPACE_VIEW);
DECLARE_PUSH_CONSTANTS(TerrainPushConstants, g_Push, TESSELLATION_BINDING_PUSH_CONSTANTS, TESSELLATION_SPACE_TERRAIN);
DECLARE_CBUFFER(TerrainConstants, c_Terrain, TESSELLATION_BINDING_TERRAIN_CONSTANTS, TESSELLATION_SPACE_TERRAIN);

StructuredBuffer<InstanceData> t_Instances : REGISTER_SRV(TESSELLATION_BINDING_INSTANCE_BUFFER, TESSELLATION_SPACE_TERRAIN);

Texture2D<float> t_HeightmapTexture : REGISTER_SRV(TESSELLATION_BINDING_SUBDIVISION_HEIGHTMAP, TESSELLATION_SPACE_TERRAIN);
SamplerState s_HeightmapSampler : REGISTER_SAMPLER(TESSELLATION_BINDING_SUBDIVISION_HEIGHTMAP_SAMPLER, TESSELLATION_SPACE_VIEW);

#include "../TerrainHelpers.hlsli"


float3 LEBSpaceToLocalSpace(float2 leb_pos)
{
    float2 pos = (leb_pos - 0.5f) * c_Terrain.TerrainExtentsAndInvExtents.xy;
    return float3(pos.x, GetTerrainHeight(leb_pos), pos.y);
}

void DecodeFaceVertices(cbt_Node node, out float3 faceVertices[3])
{
    float3x2 pos = float3x2(float2(0, 1),
        float2(0, 0),
        float2(1, 0));
    pos = leb_DecodeAttributeArray(node, pos);

    faceVertices[0] = LEBSpaceToLocalSpace(pos[0]);
    faceVertices[1] = LEBSpaceToLocalSpace(pos[1]);
    faceVertices[2] = LEBSpaceToLocalSpace(pos[2]);
}

void TransformFaceVertices(inout float3 faceVertices[3], float3x4 mat)
{
    faceVertices[0] = mul(mat, float4(faceVertices[0], 1.0f)).xyz;
    faceVertices[1] = mul(mat, float4(faceVertices[1], 1.0f)).xyz;
    faceVertices[2] = mul(mat, float4(faceVertices[2], 1.0f)).xyz;
}

float TriangleLevelOfDetail_Perspective(float3 patchVertices_WorldSpace[3])
{
    float3 v0 = mul(float4(patchVertices_WorldSpace[0], 1.0f), c_Subdivision.view.matWorldToView).xyz;
    float3 v2 = mul(float4(patchVertices_WorldSpace[2], 1.0f), c_Subdivision.view.matWorldToView).xyz;

#if 0 //  human-readable version
    vec3 edgeCenter = (v0 + v2); // division by 2 was moved to lodFactor
    vec3 edgeVector = (v2 - v0);
    float distanceToEdgeSqr = dot(edgeCenter, edgeCenter);
    float edgeLengthSqr = dot(edgeVector, edgeVector);

    return u_LodFactor + log2(edgeLengthSqr / distanceToEdgeSqr);
#else // optimized version
    float sqrMagSum = dot(v0, v0) + dot(v2, v2);
    float twoDotAC = 2.0f * dot(v0, v2);
    float distanceToEdgeSqr = sqrMagSum + twoDotAC;
    float edgeLengthSqr     = sqrMagSum - twoDotAC;

    return c_Subdivision.lodFactor + log2(edgeLengthSqr / distanceToEdgeSqr);
#endif
}

float TriangleLevelOfDetail(float3 patchVertices_WorldSpace[3])
{
    return TriangleLevelOfDetail_Perspective(patchVertices_WorldSpace);
}

float LevelOfDetail(float3 patchVertices_WorldSpace[3])
{
	// TODO: Frustum culling & terrain variance testing
    return TriangleLevelOfDetail(patchVertices_WorldSpace);
}


[numthreads(256, 1, 1)]
void split_cs(uint3 DTid : SV_DispatchThreadID)
{
    uint threadID = DTid.x;

    if (threadID < cbt_NodeCount())
    {
        cbt_Node node = cbt_DecodeNode(threadID);
        InstanceData instance = t_Instances[g_Push.startInstanceLocation];

        float3 faceVertices[3];
    	DecodeFaceVertices(node, faceVertices);
        TransformFaceVertices(faceVertices, instance.transform);

        float lod = LevelOfDetail(faceVertices);

        if (lod > 1.0f) {
            leb_SplitNode(node);
        }
    }
}

[numthreads(256, 1, 1)]
void merge_cs(uint3 DTid : SV_DispatchThreadID)
{
    uint threadID = DTid.x;

    if (threadID < cbt_NodeCount())
    {
        cbt_Node node = cbt_DecodeNode(threadID);
        InstanceData instance = t_Instances[g_Push.startInstanceLocation];

        leb_DiamondParent diamondParent = leb_DecodeDiamondParent(node);
        bool mergeBase, mergeTop;

        {
            float3 faceVertices[3];
            DecodeFaceVertices(diamondParent.base, faceVertices);
            TransformFaceVertices(faceVertices, instance.transform);

            mergeBase = LevelOfDetail(faceVertices) < 1.0f;
        }
        {
            float3 faceVertices[3];
            DecodeFaceVertices(diamondParent.top, faceVertices);
            TransformFaceVertices(faceVertices, instance.transform);

            mergeTop = LevelOfDetail(faceVertices) < 1.0f;
        }

        if (mergeTop && mergeBase)
        {
            leb_MergeNode(node, diamondParent);
        }
    }
}
