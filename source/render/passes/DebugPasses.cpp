#include "DebugPasses.h"

#include <donut/engine/View.h>
#include <donut/engine/ShaderFactory.h>

#include "nvrhi/utils.h"

using namespace donut::math;

#include "DebugTools.h"


void DebugPlanePass::Init(const std::shared_ptr<donut::engine::ShaderFactory>& shaderFactory)
{
	const char* sourceFileName = "app/DebugPlane.hlsl";

	m_VertexShader = shaderFactory->CreateAutoShader(sourceFileName, "debug_plane_vs",
		DONUT_MAKE_PLATFORM_SHADER(g_debug_plane_vs), nullptr, nvrhi::ShaderType::Vertex);
	m_PixelShader = shaderFactory->CreateAutoShader(sourceFileName, "debug_plane_ps",
		DONUT_MAKE_PLATFORM_SHADER(g_debug_plane_ps), nullptr, nvrhi::ShaderType::Pixel);

	m_ConstantBuffer = m_Device->createBuffer(nvrhi::utils::CreateVolatileConstantBufferDesc(sizeof(DebugPlaneConstants), "DebugPlaneConstants", 16));

	nvrhi::BindingSetDesc setDesc;
	setDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, m_ConstantBuffer));

	nvrhi::utils::CreateBindingSetAndLayout(m_Device, nvrhi::ShaderType::Vertex | nvrhi::ShaderType::Pixel, 0, setDesc, m_BindingLayout, m_BindingSet);
}

void DebugPlanePass::Render(nvrhi::ICommandList* commandList, const donut::engine::IView& view, nvrhi::IFramebuffer* framebuffer, donut::math::float3 PlaneNormal, donut::math::float3 PlaneOrigin)
{
	if (!m_Pipeline)
	{
		nvrhi::GraphicsPipelineDesc pipelineDesc;
		pipelineDesc.inputLayout = nullptr;
		pipelineDesc.VS = m_VertexShader;
		pipelineDesc.PS = m_PixelShader;
		pipelineDesc.bindingLayouts = {
			m_BindingLayout
		};

		pipelineDesc.primType = nvrhi::PrimitiveType::TriangleStrip;

		pipelineDesc.renderState.rasterState
			.setFrontCounterClockwise(true)
			.setCullMode(nvrhi::RasterCullMode::None)
			.setFillMode(nvrhi::RasterFillMode::Solid);

		pipelineDesc.renderState.depthStencilState
			.setDepthWriteEnable(true)
			.setDepthFunc(view.IsReverseDepth()
				? nvrhi::ComparisonFunc::GreaterOrEqual
				: nvrhi::ComparisonFunc::LessOrEqual);

		m_Pipeline = m_Device->createGraphicsPipeline(pipelineDesc, framebuffer);
	}

	DebugPlaneConstants constants;
	view.FillPlanarViewConstants(constants.View);
	constants.PlaneSize = 25.0f;
	constants.PlaneNormal = PlaneNormal;
	constants.PlaneOrigin = PlaneOrigin;
	commandList->writeBuffer(m_ConstantBuffer, &constants, sizeof(constants));

	nvrhi::GraphicsState state;
	state.pipeline = m_Pipeline;
	state.bindings = { m_BindingSet };
	state.framebuffer = framebuffer;
	state.indexBuffer = { };
	state.viewport = view.GetViewportState();
	commandList->setGraphicsState(state);

	nvrhi::DrawArguments args;
	args.vertexCount = 4;
	commandList->draw(args);
}
