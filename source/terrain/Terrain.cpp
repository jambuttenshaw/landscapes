#include "Terrain.h"

#include <donut/shaders/bindless.h>
#include "libcbt/cbt.h"

using namespace donut;
using namespace donut::math;

#include "TerrainShaders.h"

TerrainView::TerrainView(TerrainViewType viewType, uint maxDepth)
	: m_ViewType(viewType)
	, m_MaxDepth(maxDepth)
{
}

void TerrainView::Init(nvrhi::IDevice* device, nvrhi::ICommandList* commandList)
{
	cbt_Tree* cbt = cbt_CreateAtDepth(m_MaxDepth, 1);

	nvrhi::BufferDesc bufferDesc;
	bufferDesc.setByteSize(cbt_HeapByteSize(cbt))
		.setCanHaveTypedViews(true)
		.setStructStride(sizeof(uint))
		.setCanHaveUAVs(true)
		.setInitialState(nvrhi::ResourceStates::ShaderResource)
		.setKeepInitialState(true)
		.setDebugName("CBT");
	m_CBTBuffer = device->createBuffer(bufferDesc);

	commandList->writeBuffer(m_CBTBuffer, cbt_GetHeap(cbt), cbt_HeapByteSize(cbt));

	cbt_Release(cbt);
}


bool Terrain::Init(
	nvrhi::IDevice* device, 
	nvrhi::ICommandList* commandList, 
	engine::TextureCache* textureCache, 
	engine::SceneGraph* sceneGraph,
	const CreateParams& params)
{
	if (!(device && commandList && textureCache && sceneGraph))
	{
		log::error("Invalid arguments");
		return false;
	}

	m_HeightmapExtents = params.HeightmapExtents;
	m_HeightmapResolution = params.HeightmapResolution;
	m_HeightmapHeightScale = params.HeightmapHeightScale;
	m_HeightmapMetersPerPixel = params.HeightmapExtents / static_cast<float2>(m_HeightmapResolution);

	// Create root node in scene graph for terrain tile hierarchy to attach to
	m_TerrainRootNode = std::make_shared<engine::SceneGraphNode>();
	sceneGraph->Attach(sceneGraph->GetRootNode(), m_TerrainRootNode);

	// Load textures for the terrain
	if (!params.HeightmapTexturePath.empty())
	{
		std::shared_ptr<engine::LoadedTexture> heightmapTexture = textureCache->LoadTextureFromFile(params.HeightmapTexturePath, true, nullptr, commandList);
		m_HeightmapTexture = heightmapTexture->texture;

		if (!heightmapTexture->texture)
		{
			log::error("Couldn't load the texture");
			return false;
		}
	}

	// Create views
	for (uint viewType = 0; viewType < static_cast<uint>(TerrainViewType_COUNT); viewType++)
	{
		m_TerrainViews.emplace_back(static_cast<TerrainViewType>(viewType), params.CBTMaxDepth)
			.Init(device, commandList);
	}

	return true;
}

void Terrain::FillTerrainConstants(struct TerrainConstants& terrainConstants) const
{
	terrainConstants.TerrainExtentsAndInvExtents = float4(m_HeightmapExtents, 1.0f / m_HeightmapExtents);
	terrainConstants.HeightmapResolutionAndInvResolution = float4(static_cast<float2>(m_HeightmapResolution), 1.0f / static_cast<float2>(m_HeightmapResolution));
	terrainConstants.HeightScaleAndInvScale = float2(m_HeightmapHeightScale, 1.0f / m_HeightmapHeightScale);
}
