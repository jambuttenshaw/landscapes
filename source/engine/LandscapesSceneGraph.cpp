#include "LandscapesSceneGraph.h"

#include "terrain/Terrain.h"


std::shared_ptr<donut::engine::SceneGraphLeaf> LandscapesSceneTypeFactory::CreateLeaf(const std::string& type)
{
	if (type == "terrainInstance")
	{
		return CreateTerrainMeshInstance();
	}

	return SceneTypeFactory::CreateLeaf(type);
}

std::shared_ptr<TerrainMeshInfo> LandscapesSceneTypeFactory::CreateTerrainMesh()
{
	return std::make_shared<TerrainMeshInfo>();
}

std::shared_ptr<TerrainMeshInstance> LandscapesSceneTypeFactory::CreateTerrainMeshInstance()
{
	return std::make_shared<TerrainMeshInstance>();
}
