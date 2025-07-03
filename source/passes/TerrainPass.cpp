#include "TerrainPass.h"

#include <donut/engine/SceneTypes.h>
#include <donut/render/DrawStrategy.h>
#include <donut/engine/ShaderFactory.h>
#include <nvrhi/utils.h>

#include "../terrain/Terrain.h"

using namespace donut;
using namespace math;

#include <donut/shaders/gbuffer_cb.h>
#include "TerrainShaders.h"


TerrainGBufferFillPass::TerrainGBufferFillPass(nvrhi::IDevice* device, std::shared_ptr<engine::CommonRenderPasses> commonPasses)
	: m_Device(device)
	, m_CommonPasses(std::move(commonPasses))
{
	assert(m_Device->getGraphicsAPI() != nvrhi::GraphicsAPI::D3D11);
	assert(m_CommonPasses);
}

void TerrainGBufferFillPass::Init(engine::ShaderFactory& shaderFactory)
{
	m_VertexShader = CreateVertexShader(shaderFactory);
	m_PixelShader = CreatePixelShader(shaderFactory);

	m_GBufferCB = m_Device->createBuffer(nvrhi::utils::CreateVolatileConstantBufferDesc(
		sizeof(GBufferFillConstants), "GBufferFillConstants", 16
	));
	m_TerrainCB = m_Device->createBuffer(nvrhi::utils::CreateVolatileConstantBufferDesc(
		sizeof(TerrainConstants), "TerrainConstants", 16
	));

	CreateViewBindings(m_ViewBindingLayout, m_ViewBindingSet);
	m_InputBindingLayout = CreateInputBindingLayout();
	m_TerrainBindingLayout = CreateTerrainBindingLayout();
}

template <typename Key>
static nvrhi::BindingSetHandle FindOrCreateBindingSet(
	Key key,
	std::unordered_map<Key, nvrhi::BindingSetHandle>& bindingSets,
	std::function<nvrhi::BindingSetHandle(Key)> createFunc)
{
	nvrhi::BindingSetHandle inputBindingSet;
	auto it = bindingSets.find(key);
	if (it == bindingSets.end())
	{
		inputBindingSet = createFunc(key);
		bindingSets[key] = inputBindingSet;
	}
	else
	{
		inputBindingSet = it->second;
	}
	return inputBindingSet;
}

void TerrainGBufferFillPass::RenderTerrain(
	nvrhi::ICommandList* commandList,
	const engine::IView* view,
	const engine::IView* viewPrev,
	nvrhi::IFramebuffer* framebuffer,
	const Terrain* terrain,
	bool wireframe,
	donut::render::IDrawStrategy& drawStrategy
)
{
	// Update view constant buffer
	GBufferFillConstants gbufferConstants = {};
	view->FillPlanarViewConstants(gbufferConstants.view);
	viewPrev->FillPlanarViewConstants(gbufferConstants.viewPrev);
	commandList->writeBuffer(m_GBufferCB, &gbufferConstants, sizeof(gbufferConstants));

	TerrainConstants terrainConstants = {};
	terrain->FillTerrainConstants(terrainConstants);
	commandList->writeBuffer(m_TerrainCB, &terrainConstants, sizeof(terrainConstants));

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

		nvrhi::BindingSetHandle inputBindingSet = FindOrCreateBindingSet<const engine::BufferGroup*>(drawItem->buffers, m_InputBindingSets,
			[this](const engine::BufferGroup* buffers){ return CreateInputBindingSet(buffers); });
		nvrhi::BindingSetHandle terrainBindingSet = FindOrCreateBindingSet<const Terrain*>(terrain, m_TerrainBindingSets,
			[this](const Terrain* terrain) { return CreateTerrainBindingSet(terrain); });

		state.bindings = {
			m_ViewBindingSet,
			inputBindingSet,
			terrainBindingSet
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
	char const* sourceFileName = "app/TerrainShaders.hlsl";

	return shaderFactory.CreateAutoShader(sourceFileName, "gbuffer_vs",
		DONUT_MAKE_PLATFORM_SHADER(g_landscape_shaders_gbuffer_vs), nullptr, nvrhi::ShaderType::Vertex);
}

nvrhi::ShaderHandle TerrainGBufferFillPass::CreatePixelShader(engine::ShaderFactory& shaderFactory)
{
	char const* sourceFileName = "app/TerrainShaders.hlsl";

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

nvrhi::BindingLayoutHandle TerrainGBufferFillPass::CreateTerrainBindingLayout()
{
	auto bindingLayoutDesc = nvrhi::BindingLayoutDesc()
		.setVisibility(nvrhi::ShaderType::Vertex | nvrhi::ShaderType::Pixel)
		.setRegisterSpace(GBUFFER_SPACE_TERRAIN)
		.setRegisterSpaceIsDescriptorSet(true)
		.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(GBUFFER_BINDING_TERRAIN_CONSTANTS))
		.addItem(nvrhi::BindingLayoutItem::Texture_SRV(GBUFFER_BINDING_TERRAIN_HEIGHTMAP_TEXTURE));

	return m_Device->createBindingLayout(bindingLayoutDesc);
}

void TerrainGBufferFillPass::CreateViewBindings(nvrhi::BindingLayoutHandle& layout, nvrhi::BindingSetHandle& set)
{
	auto bindingLayoutDesc = nvrhi::BindingLayoutDesc()
		.setVisibility(nvrhi::ShaderType::Vertex | nvrhi::ShaderType::Pixel)
		.setRegisterSpace(GBUFFER_SPACE_VIEW)
		.setRegisterSpaceIsDescriptorSet(true)
		.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(GBUFFER_BINDING_VIEW_CONSTANTS))
		.addItem(nvrhi::BindingLayoutItem::Sampler(GBUFFER_BINDING_TERRAIN_HEIGHTMAP_SAMPLER));

	layout = m_Device->createBindingLayout(bindingLayoutDesc);

	auto bindingSetDesc = nvrhi::BindingSetDesc()
		.setTrackLiveness(true)
		.addItem(nvrhi::BindingSetItem::ConstantBuffer(GBUFFER_BINDING_VIEW_CONSTANTS, m_GBufferCB))
		.addItem(nvrhi::BindingSetItem::Sampler(GBUFFER_BINDING_TERRAIN_HEIGHTMAP_SAMPLER, m_CommonPasses->m_LinearClampSampler));

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
		m_InputBindingLayout,
		m_TerrainBindingLayout
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


nvrhi::BindingSetHandle TerrainGBufferFillPass::CreateInputBindingSet(const donut::engine::BufferGroup* buffers)
{
	auto bindingSetDesc = nvrhi::BindingSetDesc()
		.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(GBUFFER_BINDING_INSTANCE_BUFFER, buffers->instanceBuffer))
		.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(GBUFFER_BINDING_VERTEX_BUFFER, buffers->vertexBuffer))
		.addItem(nvrhi::BindingSetItem::PushConstants(GBUFFER_BINDING_PUSH_CONSTANTS, sizeof(TerrainPushConstants)));

	return m_Device->createBindingSet(bindingSetDesc, m_InputBindingLayout);
}

nvrhi::BindingSetHandle TerrainGBufferFillPass::CreateTerrainBindingSet(const Terrain* terrain)
{
	auto bindingSetDesc = nvrhi::BindingSetDesc()
		.addItem(nvrhi::BindingSetItem::ConstantBuffer(GBUFFER_BINDING_TERRAIN_CONSTANTS, m_TerrainCB))
		.addItem(nvrhi::BindingSetItem::Texture_SRV(GBUFFER_BINDING_TERRAIN_HEIGHTMAP_TEXTURE, terrain->GetHeightmapTexture()));

	return m_Device->createBindingSet(bindingSetDesc, m_TerrainBindingLayout);
}
