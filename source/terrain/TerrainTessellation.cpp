#include "TerrainTessellation.h"

#include <nvrhi/utils.h>

#include "Terrain.h"
#include "TerrainShaders.h"


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
		layoutDesc.setRegisterSpace(TESSELLATION_SPACE_CBT)
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
	const ITerrainTessellationPass& pass,
	const TerrainMeshView* terrainView)
{
	auto& bindings = m_BindingSets[terrainView];
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
}
