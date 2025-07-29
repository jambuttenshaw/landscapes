#ifndef TERRAIN_SHADERS_H
#define TERRAIN_SHADERS_H

// GBuffer rendering bindings
#define GBUFFER_SPACE_TERRAIN 3
#define GBUFFER_BINDING_TERRAIN_CONSTANTS 0
#define GBUFFER_BINDING_TERRAIN_CBT 0
#define GBUFFER_BINDING_TERRAIN_HEIGHTMAP_TEXTURE 1
#define GBUFFER_BINDING_TERRAIN_HEIGHTMAP_SAMPLER 0

// Terrain tessellation bindings
#define TESSELLATION_SPACE_CBT 0
#define TESSELLATION_BINDING_CBT 0
#define TESSELLATION_BINDING_INDIRECT_ARGS 1
#define TESSELLATION_BINDING_PUSH_CONSTANTS 0

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

struct TessellationSubdivisionPushConstants
{
	float2 Target;
};
struct TessellationSumReductionPushConstants
{
	uint PassID;
};

#endif