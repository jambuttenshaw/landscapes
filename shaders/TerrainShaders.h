#ifndef TERRAIN_SHADERS_H
#define TERRAIN_SHADERS_H

#define TERRAIN_BINDING_VIEW_CONSTANTS 3

struct TerrainConstants
{
	float2 Extents;
};

struct TerrainPushConstants
{
	uint startInstanceLocation;
};

#endif