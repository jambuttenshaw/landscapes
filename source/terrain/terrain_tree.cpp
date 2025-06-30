#include "terrain_tree.h"

#include "terrain_mesh.h"

using namespace donut;
using namespace donut::math;


TerrainTile::TerrainTile(
	std::shared_ptr<donut::engine::SceneGraphNode> parent, 
	std::shared_ptr<donut::engine::MeshInfo> terrainMesh, 
	uint32_t level
)
	: m_MeshInstance(std::move(terrainMesh))
	, m_Level(level)
{
}


Terrain::Terrain(const CreateParams& params)
	: m_HeightmapExtents(params.HeightmapExtents)
	, m_HeightmapResolution(params.HeightmapResolution)
	, m_HeightmapMetersPerPixel(params.HeightmapExtents / static_cast<float2>(m_HeightmapResolution))
	, m_TerrainResolution(params.TerrainResolution)
{
	// Calculate the maximum number of tiles needed to express the entire terrain at the highest level of detail
	uint tilesInLevel = 1;
	m_TileCount = 0;
	uint verticesAlongEdge = max(m_TerrainResolution.x, m_TerrainResolution.y);
	while (all(verticesAlongEdge <= m_HeightmapResolution))
	{
		m_TileCount += tilesInLevel;
		tilesInLevel *= 4;
		verticesAlongEdge *= 2;
	}
}

void Terrain::Init(nvrhi::IDevice* device, nvrhi::ICommandList* commandList)
{
	m_TerrainMesh = std::make_shared<TerrainMesh>(uint2{ 64, 64 });
	m_TerrainMesh->InitResources(device, commandList, static_cast<uint32_t>(m_Tiles.size()));

	m_Tiles.reserve(m_TileCount);

	{
		// Create root

		// Create tile instance
		m_Tiles[0] = std::make_shared<TerrainTile>(m_TerrainMesh->GetMeshInfo(), 0);

		// Calculate tile transform

		// Set up scene graph hierarchy

	}
}

void Terrain::CreateChildTilesFor(const std::shared_ptr<TerrainTile>& tile)
{
	
}
