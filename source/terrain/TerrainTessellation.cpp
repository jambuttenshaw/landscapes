#include "TerrainTessellation.h"

#include <nvrhi/utils.h>

#include "Terrain.h"
#include "TerrainShaders.h"
#include "engine/ViewEx.h"


ITerrainTessellationPass::ITerrainTessellationPass(nvrhi::DeviceHandle device, std::shared_ptr<donut::engine::CommonRenderPasses> commonPasses)
	: m_Device(std::move(device))
	, m_CommonPasses(std::move(commonPasses))
{
}


TerrainTessellator::TerrainTessellator(nvrhi::DeviceHandle device)
	: m_Device(std::move(device))
{
}

void TerrainTessellator::Init(donut::engine::ShaderFactory& shaderFactory)
{
	// Create shaders
	m_Shaders[Shaders_CBTDispatch] = shaderFactory.CreateAutoShader("app/terrain/tessellation/Dispatcher.hlsl", "cbt_dispatcher_cs",
		DONUT_MAKE_PLATFORM_SHADER(g_terrain_tessellation_cbt_dispatcher_cs), nullptr, nvrhi::ShaderType::Compute);
	m_Shaders[Shaders_LEBDispatch] = shaderFactory.CreateAutoShader("app/terrain/tessellation/Dispatcher.hlsl", "leb_dispatcher_cs",
		DONUT_MAKE_PLATFORM_SHADER(g_terrain_tessellation_leb_dispatcher_cs), nullptr, nvrhi::ShaderType::Compute);
	m_Shaders[Shaders_SumReductionPrePass] = shaderFactory.CreateAutoShader("app/terrain/tessellation/SumReduction.hlsl", "sum_reduction_prepass_cs",
		DONUT_MAKE_PLATFORM_SHADER(g_terrain_tessellation_sum_reduction_prepass_cs), nullptr, nvrhi::ShaderType::Compute);
	m_Shaders[Shaders_SumReduction] = shaderFactory.CreateAutoShader("app/terrain/tessellation/SumReduction.hlsl", "sum_reduction_cs",
		DONUT_MAKE_PLATFORM_SHADER(g_terrain_tessellation_sum_reduction_cs), nullptr, nvrhi::ShaderType::Compute);

	// Create binding layouts
	{
		nvrhi::BindingLayoutDesc layoutDesc;
		layoutDesc.setRegisterSpace(TESSELLATION_SPACE_TERRAIN)
			.setRegisterSpaceIsDescriptorSet(true)
			.setVisibility(nvrhi::ShaderType::Compute);

		layoutDesc.bindings = {
			nvrhi::BindingLayoutItem::StructuredBuffer_SRV(TESSELLATION_BINDING_CBT),
			nvrhi::BindingLayoutItem::StructuredBuffer_UAV(TESSELLATION_BINDING_INDIRECT_ARGS)
		};
		m_BindingLayouts[Bindings_CBTReadOnly] = m_Device->createBindingLayout(layoutDesc);

		layoutDesc.bindings = {
			nvrhi::BindingLayoutItem::PushConstants(TESSELLATION_BINDING_PUSH_CONSTANTS, sizeof(TessellationSumReductionPushConstants)),
			nvrhi::BindingLayoutItem::StructuredBuffer_UAV(TESSELLATION_BINDING_CBT)
		};
		m_BindingLayouts[Bindings_CBTReadWrite] = m_Device->createBindingLayout(layoutDesc);
	}

	// Create pipelines
	{
		nvrhi::ComputePipelineDesc psoDesc;
		psoDesc.setComputeShader(m_Shaders[Shaders_CBTDispatch])
			.addBindingLayout(m_BindingLayouts[Bindings_CBTReadOnly]);
		m_Pipelines[Shaders_CBTDispatch] = m_Device->createComputePipeline(psoDesc);
	}
	{
		nvrhi::ComputePipelineDesc psoDesc;
		psoDesc.setComputeShader(m_Shaders[Shaders_LEBDispatch])
			.addBindingLayout(m_BindingLayouts[Bindings_CBTReadOnly]);
		m_Pipelines[Shaders_LEBDispatch] = m_Device->createComputePipeline(psoDesc);
	}
	{
		nvrhi::ComputePipelineDesc psoDesc;
		psoDesc.setComputeShader(m_Shaders[Shaders_SumReductionPrePass])
			.addBindingLayout(m_BindingLayouts[Bindings_CBTReadWrite]);
		m_Pipelines[Shaders_SumReductionPrePass] = m_Device->createComputePipeline(psoDesc);
	}
	{
		nvrhi::ComputePipelineDesc psoDesc;
		psoDesc.setComputeShader(m_Shaders[Shaders_SumReduction])
			.addBindingLayout(m_BindingLayouts[Bindings_CBTReadWrite]);
		m_Pipelines[Shaders_SumReduction] = m_Device->createComputePipeline(psoDesc);
	}
}

void TerrainTessellator::ExecutePassForTerrainView(
	nvrhi::ICommandList* commandList,
	const donut::engine::IView* view,
	ITerrainTessellationPass& pass,
	const TerrainMeshView* terrainView)
{
	auto& cachedData = m_TerrainCache[terrainView];
	auto& bindings = cachedData.bindings;

	commandList->beginMarker("ExecuteTerrainTessellation");

	// Create binding sets if there are none for this terrain view
	if (!bindings[Bindings_CBTReadOnly] || !bindings[Bindings_CBTReadWrite])
	{
		nvrhi::BindingSetDesc setDesc;
		setDesc.bindings = {
			nvrhi::BindingSetItem::StructuredBuffer_SRV(TESSELLATION_BINDING_CBT, terrainView->GetCBTBuffer()),
			nvrhi::BindingSetItem::StructuredBuffer_UAV(TESSELLATION_BINDING_INDIRECT_ARGS, terrainView->GetIndirectArgsBuffer())
		};
		bindings[Bindings_CBTReadOnly] = m_Device->createBindingSet(setDesc, m_BindingLayouts[Bindings_CBTReadOnly]);

		setDesc.bindings = {
			nvrhi::BindingSetItem::PushConstants(TESSELLATION_BINDING_PUSH_CONSTANTS, sizeof(TessellationSumReductionPushConstants)),
			nvrhi::BindingSetItem::StructuredBuffer_UAV(TESSELLATION_BINDING_CBT, terrainView->GetCBTBuffer())
		};
		bindings[Bindings_CBTReadWrite] = m_Device->createBindingSet(setDesc, m_BindingLayouts[Bindings_CBTReadWrite]);
	}

	// Run compute shader dispatcher
	{
		commandList->beginMarker("CBT Dispatch");

		nvrhi::ComputeState state;
		state.pipeline = m_Pipelines[Shaders_CBTDispatch];
		state.bindings = { bindings[Bindings_CBTReadOnly] };
		commandList->setComputeState(state);

		commandList->dispatch(1);
		commandList->endMarker();
	}

	// Execute (indirectly) the tessellation pass
	{
		commandList->beginMarker("Subdivision");

		pass.SetupView(commandList, terrainView, view);

		nvrhi::ComputeState state;
		pass.SetupSubdivisionState(terrainView, static_cast<ITerrainTessellationPass::SubdivisionPassTypes>(cachedData.split), state);

		state.setIndirectParams(terrainView->GetIndirectArgsBuffer());
		commandList->setComputeState(state);

		pass.SetupPushConstants(commandList, terrainView);

		commandList->dispatchIndirect(TerrainMeshView::GetIndirectArgsDispatchOffset());

		cachedData.split = !cachedData.split;
		commandList->endMarker();
	}

	// Run sum reduction
	{
		commandList->beginMarker("Sum Reduction");

		int it = static_cast<int>(terrainView->GetMaxDepth());

		nvrhi::ComputeState state;
		{
			state.pipeline = m_Pipelines[Shaders_SumReductionPrePass];
			state.bindings = { bindings[Bindings_CBTReadWrite] };
			commandList->setComputeState(state);

			int cnt = ((1 << it) >> 5);
			int numGroup = (cnt >= 256) ? (cnt >> 8) : 1;

			TessellationSumReductionPushConstants constants = { static_cast<uint>(it) };
			commandList->setPushConstants(&constants, sizeof(constants));

			nvrhi::utils::BufferUavBarrier(commandList, terrainView->GetCBTBuffer());
			commandList->commitBarriers();

			commandList->dispatch(static_cast<uint32_t>(numGroup));

			it -= 5;
		}

		state.pipeline = m_Pipelines[Shaders_SumReduction];
		state.bindings = { bindings[Bindings_CBTReadWrite] };
		commandList->setComputeState(state);

		while (--it >= 0)
		{
			int cnt = 1 << it;
			int numGroup = (cnt >= 256) ? (cnt >> 8) : 1;

			TessellationSumReductionPushConstants constants = { static_cast<uint>(it) };
			commandList->setPushConstants(&constants, sizeof(constants));

			nvrhi::utils::BufferUavBarrier(commandList, terrainView->GetCBTBuffer());
			commandList->commitBarriers();

			commandList->dispatch(static_cast<uint32_t>(numGroup));
		}

		commandList->endMarker();
	}

	// Run draw indirect dispatcher
	{
		commandList->beginMarker("LEB Dispatch");

		nvrhi::ComputeState state;
		state.pipeline = m_Pipelines[Shaders_LEBDispatch];
		state.bindings = { bindings[Bindings_CBTReadOnly] };
		commandList->setComputeState(state);

		commandList->dispatch(1);
		commandList->endMarker();
	}

	commandList->endMarker();
	// Now the terrain can be rendered with drawIndirect
}


PrimaryViewTerrainTessellationPass::PrimaryViewTerrainTessellationPass(nvrhi::DeviceHandle device, std::shared_ptr<donut::engine::CommonRenderPasses> commonPasses)
	: ITerrainTessellationPass(std::move(device), std::move(commonPasses))
{
}

void PrimaryViewTerrainTessellationPass::Init(donut::engine::ShaderFactory& shaderFactory)
{
	m_SplitShader = shaderFactory.CreateAutoShader("app/terrain/tessellation/Subdivision.hlsl", "split_cs",
		DONUT_MAKE_PLATFORM_SHADER(g_terrain_tessellation_split_cs), nullptr, nvrhi::ShaderType::Compute);
	m_MergeShader = shaderFactory.CreateAutoShader("app/terrain/tessellation/Subdivision.hlsl", "merge_cs",
		DONUT_MAKE_PLATFORM_SHADER(g_terrain_tessellation_merge_cs), nullptr, nvrhi::ShaderType::Compute);

	{
		nvrhi::BindingLayoutDesc layoutDesc;
		layoutDesc.setVisibility(nvrhi::ShaderType::Compute)
			.setRegisterSpaceIsDescriptorSet(true)
			.setRegisterSpace(TESSELLATION_SPACE_VIEW)
			.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(TESSELLATION_BINDING_SUBDIVISION_CONSTANTS))
			.addItem(nvrhi::BindingLayoutItem::Sampler(TESSELLATION_BINDING_SUBDIVISION_HEIGHTMAP_SAMPLER));

		m_ViewBindingLayout = m_Device->createBindingLayout(layoutDesc);
	}
	{
		nvrhi::BindingLayoutDesc layoutDesc;
		layoutDesc.setVisibility(nvrhi::ShaderType::Compute)
			.setRegisterSpaceIsDescriptorSet(true)
			.setRegisterSpace(TESSELLATION_SPACE_TERRAIN)
			.addItem(nvrhi::BindingLayoutItem::PushConstants(TESSELLATION_BINDING_PUSH_CONSTANTS, sizeof(TerrainPushConstants)))
			.addItem(nvrhi::BindingLayoutItem::ConstantBuffer(TESSELLATION_BINDING_TERRAIN_CONSTANTS))
			.addItem(nvrhi::BindingLayoutItem::StructuredBuffer_UAV(TESSELLATION_BINDING_CBT))
			.addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(TESSELLATION_BINDING_INSTANCE_BUFFER))
			.addItem(nvrhi::BindingLayoutItem::Texture_SRV(TESSELLATION_BINDING_SUBDIVISION_HEIGHTMAP));

		m_TerrainBindingLayout = m_Device->createBindingLayout(layoutDesc);
	}

	{
		nvrhi::ComputePipelineDesc psoDesc;
		psoDesc.bindingLayouts = { m_TerrainBindingLayout, m_ViewBindingLayout };

		psoDesc.setComputeShader(m_SplitShader);
		m_SplitPipeline = m_Device->createComputePipeline(psoDesc);

		psoDesc.setComputeShader(m_MergeShader);
		m_MergePipeline = m_Device->createComputePipeline(psoDesc);
	}

	{
		m_ViewCB = m_Device->createBuffer(nvrhi::utils::CreateVolatileConstantBufferDesc(
			sizeof(SubdivisionConstants), "SubdivisionConstants", 16
		));
	}

	{
		nvrhi::BindingSetDesc setDesc;
		setDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(TESSELLATION_BINDING_SUBDIVISION_CONSTANTS, m_ViewCB));
		setDesc.addItem(nvrhi::BindingSetItem::Sampler(TESSELLATION_BINDING_SUBDIVISION_HEIGHTMAP_SAMPLER, m_CommonPasses->m_LinearClampSampler));

		m_ViewBindingSet = m_Device->createBindingSet(setDesc, m_ViewBindingLayout);
	}
}

void PrimaryViewTerrainTessellationPass::SetupView(nvrhi::ICommandList* commandList, const TerrainMeshView* terrainView, const donut::engine::IView* view)
{
	SubdivisionConstants constants;
	view->FillPlanarViewConstants(constants.view);
	if (auto viewEx = dynamic_cast<const PlanarViewEx*>(view))
	{
		viewEx->FillPlanarViewExConstants(constants.viewEx);
	}
	else
	{
		// Tessellation pipeline will not work correctly
		assert(false);
	}

	// Calculate LOD factor
	{
		// constants.view.matViewToClip.m11 == tan(fovy / 2)
		float tmp = (2.0f / constants.view.matViewToClip.m11)
			/ constants.view.viewportSize.y * static_cast<float>(1 << m_SubdivisionLevel)
			* m_PrimitivePixelLength;

		constants.lodFactor = -2.0f * std::log2(tmp) + 2.0f;
	}

	commandList->writeBuffer(m_ViewCB, &constants, sizeof(constants));
}

void PrimaryViewTerrainTessellationPass::SetupSubdivisionState(const TerrainMeshView* terrainView, SubdivisionPassTypes subdivisionPass, nvrhi::ComputeState& state)
{
	state.bindings = { FindOrCreateBindingSet(terrainView), m_ViewBindingSet };
	state.pipeline = subdivisionPass == Subdivision_Split ? m_SplitPipeline : m_MergePipeline;
}

void PrimaryViewTerrainTessellationPass::SetupPushConstants(nvrhi::ICommandList* commandList, const TerrainMeshView* terrainView)
{
	TerrainPushConstants constants;
	constants.startInstanceLocation = terrainView->GetInstance()->GetInstanceIndex();
	commandList->setPushConstants(&constants, sizeof(constants));
}

nvrhi::BindingSetHandle PrimaryViewTerrainTessellationPass::FindOrCreateBindingSet(const TerrainMeshView* key)
{
	nvrhi::BindingSetHandle bindingSet = m_TerrainBindingSets[key];
	if (!bindingSet)
	{
		const TerrainMeshInfo* terrainMesh = key->GetInstance()->GetTerrain();

		nvrhi::BindingSetDesc setDesc;
		setDesc.addItem(nvrhi::BindingSetItem::PushConstants(TESSELLATION_BINDING_PUSH_CONSTANTS, sizeof(TerrainPushConstants)))
			.addItem(nvrhi::BindingSetItem::ConstantBuffer(TESSELLATION_BINDING_TERRAIN_CONSTANTS, terrainMesh->GetConstantBuffer()))
			.addItem(nvrhi::BindingSetItem::StructuredBuffer_UAV(TESSELLATION_BINDING_CBT, key->GetCBTBuffer()))
			.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(TESSELLATION_BINDING_INSTANCE_BUFFER, terrainMesh->buffers->instanceBuffer.Get()))
			.addItem(nvrhi::BindingSetItem::Texture_SRV(TESSELLATION_BINDING_SUBDIVISION_HEIGHTMAP, terrainMesh->GetHeightmapTexture()));

		bindingSet = m_Device->createBindingSet(setDesc, m_TerrainBindingLayout);

		m_TerrainBindingSets[key] = bindingSet;
	}
	return bindingSet;
}
