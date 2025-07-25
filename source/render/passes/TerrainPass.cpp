#include "TerrainPass.h"

#include <donut/engine/SceneTypes.h>
#include <donut/render/DrawStrategy.h>
#include <donut/engine/ShaderFactory.h>
#include <nvrhi/utils.h>

#include "render/TerrainDrawStrategy.h"
#include "terrain/Terrain.h"

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


void TerrainGBufferFillPass::SetupView(TerrainPassContext& abstractContext, nvrhi::ICommandList* commandList, 
										const engine::IView* view, const engine::IView* viewPrev)
{
	Context& context = reinterpret_cast<Context&>(abstractContext);

	// Update view constant buffer
	GBufferFillConstants gbufferConstants = {};
	view->FillPlanarViewConstants(gbufferConstants.view);
	viewPrev->FillPlanarViewConstants(gbufferConstants.viewPrev);
	commandList->writeBuffer(m_GBufferCB, &gbufferConstants, sizeof(gbufferConstants));

	context.keyTemplate.value = 0;
	context.keyTemplate.bits.frontCounterClockwise = view->IsMirrored();
	context.keyTemplate.bits.reverseDepth = view->IsReverseDepth();
	context.keyTemplate.bits.wireframe = context.wireframe;
}

void TerrainGBufferFillPass::SetupPipeline(TerrainPassContext& abstractContext, nvrhi::RasterCullMode cullMode, nvrhi::GraphicsState& state)
{
	Context& context = reinterpret_cast<Context&>(abstractContext);

	PipelineKey key = context.keyTemplate;
	key.bits.cullMode = cullMode;

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
}

void TerrainGBufferFillPass::SetupBindings(TerrainPassContext& context, const engine::BufferGroup* buffers,
											const TerrainMeshView* terrainView, nvrhi::GraphicsState& state)
{
	// No index buffer is used for terrain
	state.indexBuffer = {};

	nvrhi::BindingSetHandle inputBindingSet = FindOrCreateBindingSet<const engine::BufferGroup*>(buffers, m_InputBindingSets,
		[this](const engine::BufferGroup* buffers) { return CreateInputBindingSet(buffers); });
	nvrhi::BindingSetHandle terrainBindingSet = FindOrCreateBindingSet<const TerrainMeshView*>(terrainView, m_TerrainBindingSets,
		[this](const TerrainMeshView* terrainView) { return CreateTerrainBindingSet(terrainView); });

	state.bindings = {
		m_ViewBindingSet,
		inputBindingSet,
		terrainBindingSet
	};
}

nvrhi::ShaderHandle TerrainGBufferFillPass::CreateVertexShader(engine::ShaderFactory& shaderFactory)
{
	char const* sourceFileName = "app/terrain/TerrainShaders.hlsl";

	return shaderFactory.CreateAutoShader(sourceFileName, "gbuffer_vs",
		DONUT_MAKE_PLATFORM_SHADER(g_landscape_shaders_gbuffer_vs), nullptr, nvrhi::ShaderType::Vertex);
}

nvrhi::ShaderHandle TerrainGBufferFillPass::CreatePixelShader(engine::ShaderFactory& shaderFactory)
{
	char const* sourceFileName = "app/terrain/TerrainShaders.hlsl";

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
		.addItem(nvrhi::BindingLayoutItem::PushConstants(GBUFFER_BINDING_PUSH_CONSTANTS, sizeof(TerrainPushConstants)));

	return m_Device->createBindingLayout(bindingLayoutDesc);
}

nvrhi::BindingLayoutHandle TerrainGBufferFillPass::CreateTerrainBindingLayout()
{
	auto bindingLayoutDesc = nvrhi::BindingLayoutDesc()
		.setVisibility(nvrhi::ShaderType::Vertex | nvrhi::ShaderType::Pixel)
		.setRegisterSpace(GBUFFER_SPACE_TERRAIN)
		.setRegisterSpaceIsDescriptorSet(true)
		.addItem(nvrhi::BindingLayoutItem::ConstantBuffer(GBUFFER_BINDING_TERRAIN_CONSTANTS))
		.addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(GBUFFER_BINDING_TERRAIN_CBT))
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
		.addItem(nvrhi::BindingSetItem::PushConstants(GBUFFER_BINDING_PUSH_CONSTANTS, sizeof(TerrainPushConstants)));

	return m_Device->createBindingSet(bindingSetDesc, m_InputBindingLayout);
}

nvrhi::BindingSetHandle TerrainGBufferFillPass::CreateTerrainBindingSet(const TerrainMeshView* terrainView)
{
	const TerrainMeshInfo* parent = terrainView->GetParent();

	auto bindingSetDesc = nvrhi::BindingSetDesc()
		.addItem(nvrhi::BindingSetItem::ConstantBuffer(GBUFFER_BINDING_TERRAIN_CONSTANTS, parent->GetConstantBuffer()))
		.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(GBUFFER_BINDING_TERRAIN_CBT, terrainView->GetCBTBuffer()))
		.addItem(nvrhi::BindingSetItem::Texture_SRV(GBUFFER_BINDING_TERRAIN_HEIGHTMAP_TEXTURE, parent->GetHeightmapTexture()));

	return m_Device->createBindingSet(bindingSetDesc, m_TerrainBindingLayout);
}


void RenderTerrainView(
	nvrhi::ICommandList* commandList,
	const donut::engine::IView* view,
	const donut::engine::IView* viewPrev,
	nvrhi::IFramebuffer* framebuffer,
	const std::shared_ptr<engine::SceneGraphNode>& rootNode,
	donut::render::IDrawStrategy& drawStrategy,
	ITerrainPass& pass,
	TerrainPassContext& passContext)
{
	commandList->beginMarker("Render Terrain View");

	drawStrategy.PrepareForView(rootNode, *view);

	pass.SetupView(passContext, commandList, view, viewPrev);

	nvrhi::GraphicsState state;
	state.framebuffer = framebuffer;
	state.viewport = view->GetViewportState();
	state.shadingRateState = view->GetVariableRateShadingState();

	while (auto abstractDrawItem = drawStrategy.GetNextItem())
	{
		auto drawItem = static_cast<const TerrainDrawItem*>(abstractDrawItem);
		if (!drawItem)
			continue;

		const TerrainMeshInstance* terrainInstance = dynamic_cast<const TerrainMeshInstance*>(drawItem->instance);
		if (!terrainInstance)
			continue;

		pass.SetupPipeline(passContext, drawItem->cullMode, state);
		pass.SetupBindings(passContext, drawItem->buffers, drawItem->terrainView, state);

		commandList->setGraphicsState(state);

		TerrainPushConstants constants{};
		constants.startInstanceLocation = drawItem->instance->GetInstanceIndex();

		commandList->setPushConstants(&constants, sizeof(constants));

		nvrhi::DrawArguments args;
		args.vertexCount = 3;
		args.instanceCount = drawItem->terrainView->GetNodeCount();
		args.startVertexLocation = 0;
		args.startIndexLocation = 0;
		args.startInstanceLocation = 0;

		commandList->draw(args);
	}

	commandList->endMarker();
}
