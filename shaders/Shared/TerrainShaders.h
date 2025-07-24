#ifndef TERRAIN_SHADERS_H
#define TERRAIN_SHADERS_H

#define GBUFFER_SPACE_TERRAIN 3
#define GBUFFER_BINDING_TERRAIN_CONSTANTS 0
#define GBUFFER_BINDING_TERRAIN_CBT 0
#define GBUFFER_BINDING_TERRAIN_HEIGHTMAP_TEXTURE 1
#define GBUFFER_BINDING_TERRAIN_HEIGHTMAP_SAMPLER 0

struct TerrainConstants
{
	float4 TerrainExtentsAndInvExtents;
	float4 HeightmapResolutionAndInvResolution;
	float2 HeightScaleAndInvScale;
};

struct TerrainPushConstants
{
	uint startInstanceLocation;
};

#endif