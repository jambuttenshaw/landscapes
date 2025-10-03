#pragma once

#include <donut/engine/SceneGraph.h>


enum struct SceneContentFlagsEx : uint32_t
{
	// Existing donut scene content flags occupy lowest 6 bits
	Terrain = 0x40
};


class TerrainMeshInfo;
class TerrainMeshInstance;


class LandscapesSceneGraph : public donut::engine::SceneGraph
{
public:
	LandscapesSceneGraph() = default;
	~LandscapesSceneGraph() = default;

	[[nodiscard]] const donut::engine::ResourceTracker<TerrainMeshInfo>& GetTerrainMeshes() const { return TerrainMeshes; }

private:
	donut::engine::ResourceTracker<TerrainMeshInfo> TerrainMeshes;
};


class LandscapesSceneTypeFactory : public donut::engine::SceneTypeFactory
{
public:
	virtual std::shared_ptr<donut::engine::SceneGraphLeaf> CreateLeaf(const std::string& type) override;

	// Terrain
	virtual std::shared_ptr<TerrainMeshInfo> CreateTerrainMesh();
	virtual std::shared_ptr<TerrainMeshInstance> CreateTerrainMeshInstance();
};
