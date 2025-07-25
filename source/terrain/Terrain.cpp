#include "Terrain.h"

#include <nvrhi/utils.h>

#include "engine/SceneGraphEx.h"

#define CBT_IMPLEMENTATION
#include "libcbt/cbt.h"

using namespace donut;
using namespace donut::math;

#include "TerrainShaders.h"

TerrainMeshView::TerrainMeshView(const TerrainMeshInfo* parent, TerrainViewType viewType, uint maxDepth, uint initDepth)
	: m_Parent(parent)
	, m_ViewType(viewType)
	, m_MaxDepth(maxDepth)
	, m_InitDepth(initDepth)
{
}

void TerrainMeshView::Init(nvrhi::IDevice* device, nvrhi::ICommandList* commandList)
{
	cbt_Tree* cbt = cbt_CreateAtDepth(m_MaxDepth, m_InitDepth);

	{
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
	}

	{
		struct IndirectArgs
		{
			nvrhi::DispatchIndirectArguments dispatchArgs;
			nvrhi::DrawIndirectArguments drawArgs;
		};

		nvrhi::BufferDesc bufferDesc;
		bufferDesc.setByteSize(sizeof(IndirectArgs))
			.setIsDrawIndirectArgs(true)
			.setCanHaveTypedViews(true)
			.setStructStride(sizeof(IndirectArgs))
			.setCanHaveUAVs(true)
			.setInitialState(nvrhi::ResourceStates::IndirectArgument)
			.setKeepInitialState(true)
			.setDebugName("CBT_IndirectArgs");
		m_IndirectArgsBuffer = device->createBuffer(bufferDesc);

		uint nodeCount = static_cast<uint>(cbt_NodeCount(cbt));

		IndirectArgs initialIndirectArgs;
		initialIndirectArgs.dispatchArgs.groupsX = nodeCount;
		initialIndirectArgs.drawArgs.vertexCount = 3;
		initialIndirectArgs.drawArgs.instanceCount = nodeCount;
		commandList->writeBuffer(m_IndirectArgsBuffer, &initialIndirectArgs, sizeof(initialIndirectArgs));
	}


	cbt_Release(cbt);
}


TerrainMeshInstance::TerrainMeshInstance(std::shared_ptr<TerrainMeshInfo> terrain)
	: MeshInstance(std::move(terrain))
{
}

box3 TerrainMeshInstance::GetLocalBoundingBox()
{
	float2 extents = Terrain().GetExtents();
	float height = Terrain().GetHeightScale();

	return {
		{ -0.5f * extents.x,   0.0f, -0.5f * extents.y },
		{  0.5f * extents.x, height,  0.5f * extents.y }
	};
}

std::shared_ptr<engine::SceneGraphLeaf> TerrainMeshInstance::Clone()
{
	return std::make_shared<TerrainMeshInstance>(std::static_pointer_cast<TerrainMeshInfo>(m_Mesh));
}

engine::SceneContentFlags TerrainMeshInstance::GetContentFlags() const
{
	return static_cast<engine::SceneContentFlags>(SceneContentFlagsEx::Terrain);
}

TerrainMeshInfo::TerrainMeshInfo(nvrhi::IDevice* device, nvrhi::ICommandList* commandList, donut::engine::TextureCache* textureCache, const CreateParams& params)
{
	assert(device && commandList && textureCache);

	// TODO: Sharing a buffer group with other terrain meshes would be ideal
	buffers = std::make_shared<engine::BufferGroup>();

	m_HeightmapExtents = params.HeightmapExtents;
	m_HeightmapResolution = params.HeightmapResolution;
	m_HeightmapHeightScale = params.HeightmapHeightScale;
	m_HeightmapMetersPerPixel = params.HeightmapExtents / static_cast<float2>(m_HeightmapResolution);

	// Load textures for the terrain
	if (!params.HeightmapTexturePath.empty())
	{
		std::shared_ptr<engine::LoadedTexture> heightmapTexture = textureCache->LoadTextureFromFile(params.HeightmapTexturePath, true, nullptr, commandList);
		m_HeightmapTexture = heightmapTexture->texture;

		if (!heightmapTexture->texture)
		{
			log::error("Couldn't load the texture");
			assert(false);
		}
	}

	// Create views
	{
		m_TerrainViews.emplace_back(this, TerrainViewType_Primary, params.CBTMaxDepth, params.CBTInitDepth)
			.Init(device, commandList);
	}

	// Create constant buffer
	m_TerrainCB = device->createBuffer(nvrhi::utils::CreateStaticConstantBufferDesc(
		sizeof(TerrainConstants), "TerrainConstants"
	));

	TerrainConstants terrainConstants;
	terrainConstants.TerrainExtentsAndInvExtents = float4(m_HeightmapExtents, 1.0f / m_HeightmapExtents);
	terrainConstants.HeightmapResolutionAndInvResolution = float4(static_cast<float2>(m_HeightmapResolution), 1.0f / static_cast<float2>(m_HeightmapResolution));
	terrainConstants.HeightScaleAndInvScale = float2(m_HeightmapHeightScale, 1.0f / m_HeightmapHeightScale);

	commandList->beginTrackingBufferState(m_TerrainCB, nvrhi::ResourceStates::CopyDest);
	commandList->writeBuffer(m_TerrainCB, &terrainConstants, sizeof(terrainConstants));
	commandList->setPermanentBufferState(m_TerrainCB, nvrhi::ResourceStates::ConstantBuffer);
}
