#include "TerrainDrawStrategy.h"

#include <donut/render/GeometryPasses.h>

#include "terrain/Terrain.h"
#include "engine/SceneGraphEx.h"
#include "engine/ViewEx.h"


void TerrainDrawStrategy::PrepareForView(const std::shared_ptr<donut::engine::SceneGraphNode>& rootNode, const donut::engine::IView& view)
{
	// Find all terrain instances in the scene
	// The draw strategy will select the terrain mesh based on the view type
	const PlanarViewEx& viewEx = dynamic_cast<const PlanarViewEx&>(view);

	m_Walker = donut::engine::SceneGraphWalker(rootNode.get());

	while (m_Walker)
	{
		auto relevantContentFlags = static_cast<donut::engine::SceneContentFlags>(SceneContentFlagsEx::Terrain);
		bool subgraphContentsRelevant = (m_Walker->GetSubgraphContentFlags() & relevantContentFlags) != donut::engine::SceneContentFlags::None;
		bool nodeContentsRelevant = (m_Walker->GetLeafContentFlags() & relevantContentFlags) != 0;

		if (subgraphContentsRelevant)
		{
			// TODO: Frustum test
			if (nodeContentsRelevant)
			{
				if (auto terrainInstance = dynamic_cast<TerrainMeshInstance*>(m_Walker->GetLeaf().get()))
				{
					TerrainDrawItem& drawItem = m_DrawItems.emplace_back();
					drawItem.instance = terrainInstance;
					drawItem.mesh = terrainInstance->GetMesh().get();
					drawItem.geometry = nullptr; // is generated dynamically from the CBT
					drawItem.material = nullptr; // TODO: (multiple materials will be required)
					drawItem.buffers = drawItem.mesh->buffers.get();
					drawItem.distanceToCamera = 0;
					drawItem.cullMode = nvrhi::RasterCullMode::None; // LEB creates vertices in opposite winding order

					drawItem.terrainView = terrainInstance->GetTerrainView(viewEx.GetTerrainViewIndex());
				}
			}
		}

		// TODO: Only allow children if node is visible
		m_Walker.Next(true);
	}

	m_NextItem = m_DrawItems.begin();
}

const donut::render::DrawItem* TerrainDrawStrategy::GetNextItem()
{
	if (m_NextItem == m_DrawItems.end())
	{
		return nullptr;
	}
	return &*(m_NextItem++);
}
