#pragma once

#include <donut/engine/SceneGraph.h>


enum struct SceneContentFlagsEx : uint32_t
{
	// Existing donut scene content flags occupy lowest 6 bits
	Terrain = 0x40
};


struct TerrainMeshInfo;
class TerrainMeshInstance;


class LandscapesSceneGraph : public donut::engine::SceneGraph
{
public:
	LandscapesSceneGraph() = default;
	~LandscapesSceneGraph() = default;

	[[nodiscard]] const std::vector<std::shared_ptr<TerrainMeshInfo>>& GetTerrainMeshes() const { return m_TerrainMeshes; }

	void AddTerrainMesh(std::shared_ptr<TerrainMeshInfo> terrainMesh) { m_TerrainMeshes.emplace_back(std::move(terrainMesh)); }

private:
	std::vector<std::shared_ptr<TerrainMeshInfo>> m_TerrainMeshes;
};


class LandscapesSceneTypeFactory : public donut::engine::SceneTypeFactory
{
public:
	virtual std::shared_ptr<donut::engine::SceneGraph> CreateGraph() override;
	virtual std::shared_ptr<donut::engine::SceneGraphLeaf> CreateLeaf(const std::string& type) override;

	// Terrain
	virtual std::shared_ptr<TerrainMeshInfo> CreateTerrainMesh();
	virtual std::shared_ptr<TerrainMeshInstance> CreateTerrainMeshInstance();
};
