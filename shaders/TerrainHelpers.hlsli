#ifndef TERRAIN_HELPERS_H
#define TERRAIN_HELPERS_H

float GetTerrainHeight(float2 texCoord)
{
	return t_HeightmapTexture.SampleLevel(s_HeightmapSampler, texCoord, 0) * c_Terrain.HeightScaleAndInvScale.x;
}

float3 GetTerrainNormal(float2 texCoord)
{
    const float3 texelOffset = float3(c_Terrain.HeightmapResolutionAndInvResolution.zw, 0);
    const float2 worldOffset = 2.0f * (c_Terrain.TerrainExtentsAndInvExtents.xy * c_Terrain.HeightmapResolutionAndInvResolution.zw);

    float2 leftTex = texCoord - texelOffset.xz;
    float2 rightTex = texCoord + texelOffset.xz;
    float2 topTex = texCoord - texelOffset.zy;
    float2 bottomTex = texCoord + texelOffset.zy;

    float leftHeight = GetTerrainHeight(leftTex);
    float rightHeight = GetTerrainHeight(rightTex);
    float topHeight = GetTerrainHeight(topTex);
    float bottomHeight = GetTerrainHeight(bottomTex);

    float3 tangent = float3(worldOffset.x, rightHeight - leftHeight, 0);
    float3 bitangent = float3(0, topHeight - bottomHeight, worldOffset.y);
    return normalize(cross(bitangent, tangent));
}

#endif