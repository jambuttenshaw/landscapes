#ifndef TERRAIN_SHADERS_H
#define TERRAIN_SHADERS_H

#include <donut/shaders/view_cb.h>
#include "ViewEx_cb.h"


// GBuffer rendering bindings
#define GBUFFER_SPACE_TERRAIN 3
#define GBUFFER_BINDING_TERRAIN_CONSTANTS 0
#define GBUFFER_BINDING_TERRAIN_CBT 0
#define GBUFFER_BINDING_TERRAIN_HEIGHTMAP_TEXTURE 1
#define GBUFFER_BINDING_TERRAIN_HEIGHTMAP_SAMPLER 0

// Terrain tessellation bindings
#define TESSELLATION_SPACE_TERRAIN 0
#define TESSELLATION_BINDING_CBT 0 // t0
#define TESSELLATION_BINDING_INDIRECT_ARGS 1 //u0
#define TESSELLATION_BINDING_PUSH_CONSTANTS 0 // b0
#define TESSELLATION_BINDING_TERRAIN_CONSTANTS 1 // b1
#define TESSELLATION_BINDING_INSTANCE_BUFFER 1 // t1
#define TESSELLATION_BINDING_SUBDIVISION_HEIGHTMAP 2 // t2

#define TESSELLATION_SPACE_VIEW 1
#define TESSELLATION_BINDING_SUBDIVISION_CONSTANTS 0 // b0
#define TESSELLATION_BINDING_SUBDIVISION_HEIGHTMAP_SAMPLER 0 // s0

struct TerrainConstants
{
	float4 TerrainExtentsAndInvExtents;
	float4 HeightmapResolutionAndInvResolution;
	float2 HeightScaleAndInvScale;
};

struct SubdivisionConstants
{
	PlanarViewConstants view;
	PlanarViewExConstants viewEx;

	float lodFactor;
};

struct TerrainPushConstants
{
	uint startInstanceLocation;
};

struct TessellationSumReductionPushConstants
{
	uint PassID;
};

#endif