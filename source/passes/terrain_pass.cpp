#include "terrain_pass.h"

#include <donut/core/log.h>

#include "../terrain/terrain_tree.h"
#include "donut/engine/SceneTypes.h"
#include "donut/render/DrawStrategy.h"
#include "nvrhi/utils.h"

using namespace donut;
using namespace math;

#include <donut/shaders/gbuffer_cb.h>

struct TerrainPushConstants
{
	uint startInstanceLocation;
};


TerrainGBufferFillPass::TerrainGBufferFillPass(nvrhi::IDevice* device)
	: m_Device(device)
{
	assert(m_Device->getGraphicsAPI() != nvrhi::GraphicsAPI::D3D11);
}

void TerrainGBufferFillPass::Init(engine::ShaderFactory& shaderFactory)
{
	m_VertexShader = CreateVertexShader(shaderFactory);
	m_PixelShader = CreatePixelShader(shaderFactory);

	m_GBufferCB = m_Device->createBuffer(nvrhi::utils::CreateVolatileConstantBufferDesc(
		sizeof(GBufferFillConstants), "GBufferFillConstants", 16
	));

	CreateViewBindings(m_ViewBindingLayout, m_ViewBindingSet);
	m_InputBindingLayout = CreateInputBindingLayout();
}

void TerrainGBufferFillPass::RenderTerrain(
	nvrhi::ICommandList* commandList,
	const engine::IView* view,
	const engine::IView* viewPrev,
	nvrhi::IFramebuffer* framebuffer,
	bool wireframe,
	donut::render::IDrawStrategy& drawStrategy
)
{
	// Update view constant buffer
	GBufferFillConstants gbufferConstants = {};
	view->FillPlanarViewConstants(gbufferConstants.view);
	viewPrev->FillPlanarViewConstants(gbufferConstants.viewPrev);
	commandList->writeBuffer(m_GBufferCB, &gbufferConstants, sizeof(gbufferConstants));

	// setup graphics state
	nvrhi::GraphicsState state;
	state.framebuffer = framebuffer;
	state.viewport = view->GetViewportState();
	state.shadingRateState = view->GetVariableRateShadingState();

	PipelineKey keyTemplate;
	keyTemplate.value = 0;
	keyTemplate.bits.frontCounterClockwise = view->IsMirrored();
	keyTemplate.bits.reverseDepth = view->IsReverseDepth();
	keyTemplate.bits.wireframe = wireframe;

	while (const render::DrawItem* drawItem = drawStrategy.GetNextItem())
	{
		PipelineKey key = keyTemplate;
		key.bits.cullMode = drawItem->cullMode;

		// Get graphics pipeline
		nvrhi::GraphicsPipelineHandle& pipeline = m_Pipelines[key.value];

		if (!pipeline)
		{
			std::lock_guard lock(m_Mutex);

			if (!pipeline)
			{
				pipeline = CreateGraphicsPipeline(key, state.framebuffer);
			}

			if (!pipeline)
			{
				return;
			}
		}

		assert(pipeline->getFramebufferInfo() == state.framebuffer->getFramebufferInfo());

		state.pipeline = pipeline;

		// No index buffer is used for terrain
		state.indexBuffer = { drawItem->buffers->indexBuffer, nvrhi::Format::R32_UINT, 0 };

		nvrhi::BindingSetHandle inputBindingSet;
		auto it = m_InputBindingSets.find(drawItem->buffers);
		if (it == m_InputBindingSets.end())
		{
			auto bindingSetDesc = nvrhi::BindingSetDesc()
				.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(GBUFFER_BINDING_INSTANCE_BUFFER, drawItem->buffers->instanceBuffer))
				.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(GBUFFER_BINDING_VERTEX_BUFFER, drawItem->buffers->vertexBuffer))
				.addItem(nvrhi::BindingSetItem::PushConstants(GBUFFER_BINDING_PUSH_CONSTANTS, sizeof(TerrainPushConstants)));

			inputBindingSet = m_Device->createBindingSet(bindingSetDesc, m_InputBindingLayout);
			m_InputBindingSets[drawItem->buffers] = inputBindingSet;
		}
		else
		{
			inputBindingSet = it->second;
		}

		state.bindings = {
			m_ViewBindingSet,
			inputBindingSet
		};

		commandList->setGraphicsState(state);

		TerrainPushConstants constants{};
		auto terrainMeshInstance = dynamic_cast<const TerrainMeshInstance*>(drawItem->instance);
		assert(terrainMeshInstance);
		constants.startInstanceLocation = terrainMeshInstance->GetTerrainTileIndex();

		commandList->setPushConstants(&constants, sizeof(constants));

		nvrhi::DrawArguments args;
		args.vertexCount = drawItem->geometry->numIndices;
		args.instanceCount = 1;
		args.startVertexLocation = 0;
		args.startIndexLocation = 0;
		args.startInstanceLocation = 0;

		commandList->drawIndexed(args);
	}
}

nvrhi::ShaderHandle TerrainGBufferFillPass::CreateVertexShader(engine::ShaderFactory& shaderFactory)
{
	char const* sourceFileName = "app/landscape_shaders.hlsl";

	return shaderFactory.CreateAutoShader(sourceFileName, "gbuffer_vs",
		DONUT_MAKE_PLATFORM_SHADER(g_landscape_shaders_gbuffer_vs), nullptr, nvrhi::ShaderType::Vertex);
}

nvrhi::ShaderHandle TerrainGBufferFillPass::CreatePixelShader(engine::ShaderFactory& shaderFactory)
{
	char const* sourceFileName = "app/landscape_shaders.hlsl";

	return shaderFactory.CreateAutoShader(sourceFileName, "gbuffer_ps", 
		DONUT_MAKE_PLATFORM_SHADER(g_landscape_shaders_gbuffer_ps), nullptr, nvrhi::ShaderType::Pixel);

}

nvrhi::BindingLayoutHandle TerrainGBufferFillPass::CreateInputBindingLayout()
{
	auto bindingLayoutDesc = nvrhi::BindingLayoutDesc()
		.setVisibility(nvrhi::ShaderType::Vertex | nvrhi::ShaderType::Pixel)
		.setRegisterSpace(GBUFFER_SPACE_INPUT)
		.setRegisterSpaceIsDescriptorSet(true)
		.addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(GBUFFER_BINDING_INSTANCE_BUFFER))
		.addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(GBUFFER_BINDING_VERTEX_BUFFER))
		.addItem(nvrhi::BindingLayoutItem::PushConstants(GBUFFER_BINDING_PUSH_CONSTANTS, sizeof(TerrainPushConstants)));

	return m_Device->createBindingLayout(bindingLayoutDesc);
}

void TerrainGBufferFillPass::CreateViewBindings(nvrhi::BindingLayoutHandle& layout, nvrhi::BindingSetHandle& set)
{
	auto bindingLayoutDesc = nvrhi::BindingLayoutDesc()
		.setVisibility(nvrhi::ShaderType::Vertex | nvrhi::ShaderType::Pixel)
		.setRegisterSpace(GBUFFER_SPACE_VIEW)
		.setRegisterSpaceIsDescriptorSet(true)
		.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(GBUFFER_BINDING_VIEW_CONSTANTS));

	layout = m_Device->createBindingLayout(bindingLayoutDesc);

	auto bindingSetDesc = nvrhi::BindingSetDesc()
		.setTrackLiveness(true)
		.addItem(nvrhi::BindingSetItem::ConstantBuffer(GBUFFER_BINDING_VIEW_CONSTANTS, m_GBufferCB));

	set = m_Device->createBindingSet(bindingSetDesc, layout);
}

nvrhi::GraphicsPipelineHandle TerrainGBufferFillPass::CreateGraphicsPipeline(PipelineKey key, nvrhi::IFramebuffer* sampleFramebuffer)
{
	nvrhi::GraphicsPipelineDesc pipelineDesc;
	pipelineDesc.inputLayout = nullptr;
	pipelineDesc.VS = m_VertexShader;
	pipelineDesc.PS = m_PixelShader;
	pipelineDesc.bindingLayouts = {
		m_ViewBindingLayout,
		m_InputBindingLayout
	};

	pipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;

	pipelineDesc.renderState.rasterState
		.setFrontCounterClockwise(key.bits.frontCounterClockwise)
		.setCullMode(key.bits.cullMode)
		.setFillMode(key.bits.wireframe ? nvrhi::RasterFillMode::Wireframe : nvrhi::RasterFillMode::Solid);

	pipelineDesc.renderState.depthStencilState
		.setDepthWriteEnable(true)
		.setDepthFunc(key.bits.reverseDepth
			? nvrhi::ComparisonFunc::GreaterOrEqual
			: nvrhi::ComparisonFunc::LessOrEqual);

	return m_Device->createGraphicsPipeline(pipelineDesc, sampleFramebuffer);
}
