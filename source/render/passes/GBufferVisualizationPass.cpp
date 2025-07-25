#include "GBufferVisualizationPass.h"

#include <donut/engine/View.h>
#include <donut/engine/ShaderFactory.h>
#include <donut/render/GBuffer.h>


using namespace donut::math;


GBufferVisualizationPass::GBufferVisualizationPass(nvrhi::IDevice* device)
	: m_Device(device)
	, m_BindingSets(device)
{
}

void GBufferVisualizationPass::Init(const std::shared_ptr<donut::engine::ShaderFactory>& shaderFactory)
{
	{
		nvrhi::BindingLayoutDesc layoutDesc;
		layoutDesc.setVisibility(nvrhi::ShaderType::Compute)
			.setRegisterSpace(0)
			.setRegisterSpaceIsDescriptorSet(true)
			.addItem(nvrhi::BindingLayoutItem::PushConstants(0, sizeof(uint2)))
			.addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
			.addItem(nvrhi::BindingLayoutItem::Texture_SRV(1))
			.addItem(nvrhi::BindingLayoutItem::Texture_SRV(2))
			.addItem(nvrhi::BindingLayoutItem::Texture_SRV(3))
			.addItem(nvrhi::BindingLayoutItem::Texture_SRV(4))
			.addItem(nvrhi::BindingLayoutItem::Texture_UAV(0));
		m_BindingLayout = m_Device->createBindingLayout(layoutDesc);
	}

	m_Shaders[VisualizationMode_Unlit] =   shaderFactory->CreateAutoShader("app/GBufferVisualization.hlsl", "visualize_unlit_cs",
											DONUT_MAKE_PLATFORM_SHADER(g_visualize_unlit_cs), nullptr, nvrhi::ShaderType::Compute);
	m_Shaders[VisualizationMode_Normals] = shaderFactory->CreateAutoShader("app/GBufferVisualization.hlsl", "visualize_normals_cs",
										DONUT_MAKE_PLATFORM_SHADER(g_visualize_normals_cs), nullptr, nvrhi::ShaderType::Compute);

	nvrhi::ComputePipelineDesc psoDesc;
	psoDesc.bindingLayouts = { m_BindingLayout };

	for (uint i = 0; i < VisualizationMode_COUNT; i++)
	{
		psoDesc.CS = m_Shaders[i];
		m_Pipelines[i] = m_Device->createComputePipeline(psoDesc);
	}
}

void GBufferVisualizationPass::Render(
	nvrhi::ICommandList* commandList,
	const donut::engine::IView& view,
	const donut::render::GBufferRenderTargets& gbuffer,
	VisualizationMode mode,
	nvrhi::ITexture* output)
{
	assert(output);

	commandList->beginMarker("GBufferVisualization");

	nvrhi::TextureSubresourceSet viewSubresources = view.GetSubresources();
	nvrhi::BindingSetDesc bindingSetDesc;
	bindingSetDesc.bindings = {
		nvrhi::BindingSetItem::PushConstants(0, sizeof(uint2)),

		nvrhi::BindingSetItem::Texture_SRV(0, gbuffer.Depth, nvrhi::Format::UNKNOWN, viewSubresources),
		nvrhi::BindingSetItem::Texture_SRV(1, gbuffer.GBufferDiffuse, nvrhi::Format::UNKNOWN, viewSubresources),
		nvrhi::BindingSetItem::Texture_SRV(2, gbuffer.GBufferSpecular, nvrhi::Format::UNKNOWN, viewSubresources),
		nvrhi::BindingSetItem::Texture_SRV(3, gbuffer.GBufferNormals, nvrhi::Format::UNKNOWN, viewSubresources),
		nvrhi::BindingSetItem::Texture_SRV(4, gbuffer.GBufferEmissive, nvrhi::Format::UNKNOWN, viewSubresources),

		nvrhi::BindingSetItem::Texture_UAV(0, output)
	};

	auto bindingSet = m_BindingSets.GetOrCreateBindingSet(bindingSetDesc, m_BindingLayout);

	nvrhi::ComputeState state;
	state.pipeline = m_Pipelines[mode];
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
