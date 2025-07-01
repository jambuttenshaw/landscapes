#pragma once

#include <nvrhi/nvrhi.h>
#include <donut/core/math/math.h>
#include <donut/engine/SceneGraph.h>

using namespace donut::math;


class TerrainMeshInstance : public donut::engine::MeshInstance
{
public:
	explicit TerrainMeshInstance(std::shared_ptr<donut::engine::MeshInfo> mesh, uint tileIndex)
		: MeshInstance(std::move(mesh))
		, m_TerrainTileIndex(tileIndex)
	{
	}

	inline uint GetTerrainTileIndex() const { return m_TerrainTileIndex; }

private:
	// Index into the terrain tile instance buffer for looking up instance data
	uint m_TerrainTileIndex;
};


class TerrainTile
{
public:
	static constexpr uint InvalidIndex = static_cast<uint>(-1);
public:
	TerrainTile(
		donut::engine::SceneGraph* sceneGraph,
		const std::shared_ptr<donut::engine::SceneGraphNode>& parent,
		std::shared_ptr<donut::engine::MeshInfo> terrainMesh,
		uint level,
		uint tileIndex,
		uint parentIndex
	);

	inline uint GetLevel() const { return m_Level; }
	inline uint GetTileIndex() const { return m_TileIndex; }

	inline uint GetParentIndex() const { return m_ParentIndex; }
	inline bool IsRoot() const { return m_ParentIndex == InvalidIndex; }

	inline uint GetChildIndex(uint child) const { return m_ChildrenIndices.at(child); }
	inline void SetChildIndex(uint child, uint index) { m_ChildrenIndices[child] = index; }

	// Works on assumption that node either has all children or no children
	inline bool HasChildren() const { return m_ChildrenIndices.at(0) != InvalidIndex; }

	inline const std::shared_ptr<donut::engine::SceneGraphNode>& GetGraphNode() const { return m_Node; }
	inline const std::shared_ptr<TerrainMeshInstance>& GetMeshInstance() const { return m_MeshInstance; }

private:
	// Which level of the tree this tile exists at
	uint m_Level;
	// The index into the global tile array for this tile (for looking up instance data in instance buffer)
	uint m_TileIndex;

	uint m_ParentIndex;
	std::array<uint, 4> m_ChildrenIndices;

	// The mesh instance is a leaf of the node in the tree
	std::shared_ptr<donut::engine::SceneGraphNode> m_Node;
	std::shared_ptr<TerrainMeshInstance> m_MeshInstance;
};


// TODO: This will be / contain a scene graph node
class Terrain
{
public:
	struct CreateParams
	{
		float2 HeightmapExtents = float2{ 512, 512 };
		uint2 HeightmapResolution = uint2{ 256, 256 };

		uint2 TerrainResolution = uint2{ 16, 16 };
	};

	Terrain(const CreateParams& params);

	void Init(nvrhi::IDevice* device, nvrhi::ICommandList* commandList, donut::engine::SceneGraph* sceneGraph);


	inline uint GetNumLevels() const { return m_NumLevels; }
	void GetAllTilesAtLevel(uint level, std::vector<TerrainTile*>& outTiles) const;

private:

	void CreateMesh(nvrhi::IDevice* device, nvrhi::ICommandList* commandList, uint tileCount);

	uint CreateSubtreeFor(
		donut::engine::SceneGraph* sceneGraph,
		std::vector<struct InstanceData>& instanceData,
		TerrainTile* parent,
		uint level,
		float3 scale,
		float3 translation
	);

	void GetAllTilesAtLevel_Impl(uint nodeIndex, uint level, std::vector<TerrainTile*>& outTiles) const;

private:
	// The width and height of the entire terrain
	float2 m_HeightmapExtents;
	// The number of pixels in the heightmap
	uint2 m_HeightmapResolution;
	// The number of meters each pixel in the heightmap corresponds to
	float2 m_HeightmapMetersPerPixel;

	// The same mesh is shared by all terrain tiles
	uint2 m_TerrainResolution;
	std::shared_ptr<donut::engine::BufferGroup> m_Buffers;
	std::shared_ptr<donut::engine::MeshInfo> m_TerrainMeshInfo;

	// Terrain tile instances
	std::vector<std::shared_ptr<TerrainTile>> m_Tiles;
	// Num of levels in the tree
	uint m_NumLevels;

	// Scene graph objects
	std::shared_ptr<donut::engine::SceneGraphNode> m_TerrainRootNode;
};
