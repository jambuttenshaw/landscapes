#include "terrain_tree.h"

#include "terrain_mesh.h"

using namespace donut;
using namespace donut::math;


Terrain::Terrain(const CreateParams& params)
	: m_HeightmapExtents(params.HeightmapExtents)
	, m_HeightmapResolution(params.HeightmapResolution)
	, m_HeightmapMetersPerPixel(params.HeightmapExtents / static_cast<float2>(m_HeightmapResolution))
	, m_TerrainResolution(params.TerrainResolution)
{
	// Calculate the maximum number of tiles needed to express the entire terrain at the highest level of detail
	uint tilesInLevel = 1;
	uint totalTiles = 0;
	uint verticesAlongEdge = max(m_TerrainResolution.x, m_TerrainResolution.y);
	while (all(verticesAlongEdge <= m_HeightmapResolution))
	{
		totalTiles += tilesInLevel;
		tilesInLevel *= 4;
		verticesAlongEdge *= 2;
	}

	m_Tiles.resize(totalTiles);

	// Tile data is populated init so we can populate the instance buffer with transforms
}

void Terrain::Init(nvrhi::IDevice* device, nvrhi::ICommandList* commandList)
{
	m_TerrainMesh = std::make_shared<TerrainMesh>(uint2{ 64, 64 });
	m_TerrainMesh->InitResources(device, commandList, m_Tiles);
}
