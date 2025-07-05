#include "GBufferVisualizationPass.h"

#include <donut/engine/View.h>
#include <donut/engine/ShaderFactory.h>

#include "donut/render/GBuffer.h"

using namespace donut::math;


GBufferVisualizationPass::GBufferVisualizationPass(nvrhi::IDevice* device, std::shared_ptr<donut::engine::CommonRenderPasses> commonPasses)
	: m_Device(device)
	, m_BindingSets(device)
	, m_CommonPasses(std::move(commonPasses))
{
}

void GBufferVisualizationPass::Init(const std::shared_ptr<donut::engine::ShaderFactory>& shaderFactory)
{
	{
		nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings = {
			nvrhi::BindingLayoutItem::PushConstants(0, sizeof(uint2)),
            nvrhi::BindingLayoutItem::Texture_SRV(0),
            nvrhi::BindingLayoutItem::Sampler(0),
			nvrhi::BindingLayoutItem::Texture_UAV(0)
        };
		m_BindingLayout = m_Device->createBindingLayout(layoutDesc);
	}

	m_ComputeShader = CreateComputeShader(*shaderFactory);

	nvrhi::ComputePipelineDesc psoDesc;
	psoDesc.CS = m_ComputeShader;
	psoDesc.bindingLayouts = { m_BindingLayout };

	m_Pso = m_Device->createComputePipeline(psoDesc);
}

nvrhi::ShaderHandle GBufferVisualizationPass::CreateComputeShader(donut::engine::ShaderFactory& shaderFactory)
{
	return shaderFactory.CreateAutoShader(
		"app/Debug/GBufferVisualization.hlsl",
		"visualize_normals_cs",
		DONUT_MAKE_PLATFORM_SHADER(g_visualize_normals_cs),
		nullptr,
		nvrhi::ShaderType::Compute);
}

void GBufferVisualizationPass::Render(
	nvrhi::ICommandList* commandList,
	const donut::engine::IView& view,
	const donut::render::GBufferRenderTargets& gbuffer,
	nvrhi::ITexture* output)
{
	assert(output);

	commandList->beginMarker("GBufferVisualization");

	nvrhi::BindingSetDesc bindingSetDesc;
	bindingSetDesc.bindings = {
		nvrhi::BindingSetItem::PushConstants(0, sizeof(uint2)),
		nvrhi::BindingSetItem::Texture_SRV(0, gbuffer.GBufferNormals),
		nvrhi::BindingSetItem::Sampler(0, m_CommonPasses->m_LinearClampSampler),
		nvrhi::BindingSetItem::Texture_UAV(0, output)
	};

	auto bindingSet = m_BindingSets.GetOrCreateBindingSet(bindingSetDesc, m_BindingLayout);

	nvrhi::ComputeState state;
	state.pipeline = m_Pso;
	state.bindings = { bindingSet };
	commandList->setComputeState(state);

	auto viewExtent = view.GetViewExtent();
	uint2 viewDims{ static_cast<uint>(viewExtent.width()), static_cast<uint>(viewExtent.height()) };
	commandList->setPushConstants(&viewDims, sizeof(viewDims));

	commandList->dispatch(
		dm::div_ceil(viewExtent.width(), 16),
		dm::div_ceil(viewExtent.height(), 16));

	commandList->endMarker();
}

void GBufferVisualizationPass::ResetBindingCache()
{
	m_BindingSets.Clear();
}
