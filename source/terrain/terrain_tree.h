#pragma once

#include <nvrhi/nvrhi.h>
#include <donut/core/math/math.h>
#include <donut/engine/SceneGraph.h>

using namespace donut::math;


// TODO: This will be / contain a scene graph node/leaf
class TerrainTile
{
public:
	TerrainTile(
		std::shared_ptr<donut::engine::MeshInfo>,
		uint32_t level
	);

private:
	// Which level of the tree this tile exists at
	uint32_t m_Level;

	// The mesh instance is a leaf of the node in the tree
	std::shared_ptr<donut::engine::SceneGraphNode> m_Node;
	std::shared_ptr<donut::engine::MeshInstance> m_MeshInstance;
};


// TODO: This will be / contain a scene graph node
class Terrain
{
public:
	struct CreateParams
	{
		float2 HeightmapExtents = float2{ 512, 512 };
		uint2 HeightmapResolution = uint2{ 256, 256 };

		uint2 TerrainResolution = uint2{ 64, 64 };
	};

	Terrain(const CreateParams& params);

	void Init(nvrhi::IDevice* device, nvrhi::ICommandList* commandList);

private:
	void CreateChildTilesFor(const std::shared_ptr<TerrainTile>& tile);

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
	uint m_TileCount;
	std::vector<std::shared_ptr<TerrainTile>> m_Tiles;

	// Scene graph objects

};
