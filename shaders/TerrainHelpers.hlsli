#ifndef TERRAIN_HELPERS_H
#define TERRAIN_HELPERS_H

float GetTerrainHeight(float2 texCoord)
{
	return t_HeightmapTexture.SampleLevel(s_HeightmapSampler, texCoord, 0) * c_Terrain.HeightScale;
}

#endif