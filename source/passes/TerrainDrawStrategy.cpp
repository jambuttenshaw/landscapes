#include "TerrainDrawStrategy.h"

#include <donut/render/GeometryPasses.h>

#include <algorithm>

#include "../terrain/Terrain.h"


void TerrainDrawStrategy::PrepareForView(const std::shared_ptr<donut::engine::SceneGraphNode>& rootNode, const donut::engine::IView& view)
{
	
}

const donut::render::DrawItem* TerrainDrawStrategy::GetNextItem()
{
	if (m_NextItem == m_DrawItems.end())
	{
		return nullptr;
	}
	return &*(m_NextItem++);
}

void TerrainDrawStrategy::SetData(const Terrain* terrain, donut::math::uint levelToDraw)
{
	assert(terrain != nullptr);
	assert(levelToDraw < terrain->GetNumLevels());

	std::vector<TerrainTile*> tilesToDraw;
	terrain->GetAllTilesAtLevel(levelToDraw, tilesToDraw);

	m_DrawItems.reserve(tilesToDraw.size());
	for (const auto& tile : tilesToDraw)
	{
		donut::render::DrawItem& drawItem = m_DrawItems.emplace_back();
		drawItem.instance = tile->GetMeshInstance().get();
		drawItem.mesh = drawItem.instance->GetMesh().get();
		drawItem.geometry = drawItem.mesh->geometries[0].get();
		drawItem.material = drawItem.geometry->material.get();
		drawItem.buffers = drawItem.mesh->buffers.get();
		drawItem.distanceToCamera = 0;
		drawItem.cullMode = nvrhi::RasterCullMode::None;
	}

	m_NextItem = m_DrawItems.begin();
}
