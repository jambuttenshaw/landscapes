#include "Terrain.h"

#include <nvrhi/utils.h>
#include <json/value.h>

#include "engine/LandscapesSceneGraph.h"

#define CBT_IMPLEMENTATION
#include "libcbt/cbt.h"

using namespace donut;
using namespace donut::math;

#include "TerrainShaders.h"

TerrainMeshView::TerrainMeshView(const TerrainMeshInstance* parent, const TerrainMeshViewDesc& desc)
	: m_Instance(parent)
	, m_MaxDepth(desc.MaxDepth)
	, m_InitDepth(desc.InitDepth)
	, m_TessellationScheme(desc.TessellationScheme)
{
}

void TerrainMeshView::CreateBuffers(nvrhi::IDevice* device, nvrhi::ICommandList* commandList)
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


TerrainMeshInstance::TerrainMeshInstance()
	: MeshInstance(nullptr)
{}

TerrainMeshInstance::TerrainMeshInstance(std::shared_ptr<TerrainMeshInfo> terrain)
	: MeshInstance(std::move(terrain))
{
	Create();
}

void TerrainMeshInstance::Load(const Json::Value& node)
{
	// Fetch TerrainMeshInfo from scene graph

	size_t terrainMeshIndex = 0;
	const auto& terrainIndex = node["terrain"];
	if (!terrainIndex.empty())
	{
		terrainMeshIndex = terrainIndex.asUInt64();
	}

	// Get graph
	auto nodePtr = GetNodeSharedPtr();
	if (!nodePtr)
	{
		log::fatal("Failed to get node!");
		return;
	}

	auto graph = std::dynamic_pointer_cast<LandscapesSceneGraph>(nodePtr->GetGraph());
	if (!graph)
	{
		log::fatal("Failed to get graph!");
	}
			
	// check valid
	const auto& terrainMeshes = graph->GetTerrainMeshes();
	if (terrainMeshIndex >= terrainMeshes.size())
	{
		log::error("Failed to load terrain mesh instance: terrain mesh index was invalid (Index = %d)", terrainMeshIndex);
		return;
	}

	// Must re-register leaf to ensure mesh is caught by graph
	// This can be achieved by detaching and reattaching node
	auto parent = nodePtr->GetParent();
	graph->Detach(nodePtr);
	// Update mesh
	m_Mesh = terrainMeshes.at(terrainMeshIndex);
	// Reattach
	graph->Attach(parent ? parent->shared_from_this() : nullptr, nodePtr);

	Create();
}

void TerrainMeshInstance::Create()
{
	if (!m_Mesh)
	{
		log::fatal("Create called before mesh was assigned!");
	}

	for (const auto& terrainView : Terrain().TerrainViews)
	{
		m_TerrainViews.emplace_back(this, terrainView);
	}
}

void TerrainMeshInstance::CreateBuffers(nvrhi::IDevice* device, nvrhi::ICommandList* commandList)
{
	for (auto& view : m_TerrainViews)
	{
		view.CreateBuffers(device, commandList);
	}
}

box3 TerrainMeshInstance::GetLocalBoundingBox()
{
	float2 extents = Terrain().HeightmapExtents;
	float height = Terrain().HeightmapHeightScale;

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
