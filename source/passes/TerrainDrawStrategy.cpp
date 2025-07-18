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

void TerrainDrawStrategy::SetData(const Terrain* terrain)
{
	assert(terrain != nullptr);

	donut::render::DrawItem& drawItem = m_DrawItems.emplace_back();
	drawItem.instance = ;
	drawItem.mesh = ;
	drawItem.geometry = ;
	drawItem.material = ;
	drawItem.buffers = ;
	drawItem.distanceToCamera = 0;
	drawItem.cullMode = nvrhi::RasterCullMode::None;

	m_NextItem = m_DrawItems.begin();
}
