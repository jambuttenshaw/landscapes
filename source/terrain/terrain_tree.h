#pragma once

#include <nvrhi/nvrhi.h>
#include <donut/core/math/math.h>
#include <donut/engine/SceneGraph.h>

using namespace donut::math;


// TODO: This will be / contain a scene graph node/leaf
class TerrainTile
{
public:
	TerrainTile() : m_Level(-1) {}
	TerrainTile(uint32_t level) : m_Level(level) {}

private:
	// Which level of the tree this tile exists at
	uint32_t m_Level;
	// The mesh instance to render this terrain tile
	// The important thing is the transform
	// This is stored in the instance buffer and is looked up via the TerrainTile index
	std::shared_ptr<donut::engine::MeshInstance> m_Instance;

	// TODO: Don't know if I actually need to keep track of children with pointers since everything is in a contiguous array
	TerrainTile* m_Parent = nullptr;
	std::array<TerrainTile*, 4> m_Children{ nullptr };
};


// TODO: This will be / contain a scene graph node
class Terrain
{
public:
	struct CreateParams
	{
		float2 HeightmapExtents = float2{ 674.8f, 674.8f };
		uint2 HeightmapResolution = uint2{ 256, 256 };

		uint2 TerrainResolution = uint2{ 64, 64 };
	};

	Terrain(const CreateParams& params);

	void Init(nvrhi::IDevice* device, nvrhi::ICommandList* commandList);

private:
	// The width and height of the entire terrain
	float2 m_HeightmapExtents;
	// The number of pixels in the heightmap
	uint2 m_HeightmapResolution;
	// The number of meters each pixel in the heightmap corresponds to
	float2 m_HeightmapMetersPerPixel;

	// The same mesh is shared by all terrain tiles
	uint2 m_TerrainResolution;
	std::shared_ptr<class TerrainMesh> m_TerrainMesh;

	// Terrain tile instances
	std::vector<TerrainTile> m_Tiles;
};
