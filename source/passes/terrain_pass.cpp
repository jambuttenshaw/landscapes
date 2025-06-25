#include "terrain_pass.h"

#include <donut/core/log.h>

#include "donut/engine/SceneTypes.h"
#include "donut/render/DrawStrategy.h"
#include "nvrhi/utils.h"

using namespace donut;
using namespace math;

#include <donut/shaders/gbuffer_cb.h>


TerrainGBufferFillPass::TerrainGBufferFillPass(nvrhi::IDevice* device, std::shared_ptr<engine::CommonRenderPasses> commonPasses)
	: m_Device(device)
	, m_CommonPasses(std::move(commonPasses))
{
	assert(m_Device->getGraphicsAPI() != nvrhi::GraphicsAPI::D3D11);
	m_SupportedViewTypes = engine::ViewType::PLANAR;
}

void TerrainGBufferFillPass::Init(engine::ShaderFactory& shaderFactory, const CreateParameters& params)
{
	m_VertexShader = CreateVertexShader(shaderFactory, params);
	m_PixelShader = CreatePixelShader(shaderFactory, params);

	if (params.materialBindings)
	{
		m_MaterialBindings = params.materialBindings;
	}
	else
	{
		m_MaterialBindings = CreateMaterialBindingCache(*m_CommonPasses);
	}

	m_GBufferCB = m_Device->createBuffer(nvrhi::utils::CreateVolatileConstantBufferDesc(
		sizeof(GBufferFillConstants), "GBufferFillConstants", params.numConstantBufferVersions
	));

	CreateViewBindings(m_ViewBindingLayout, m_ViewBindingSet, params);
	m_InputBindingLayout = CreateInputBindingLayout(params);
}

engine::ViewType::Enum TerrainGBufferFillPass::GetSupportedViewTypes() const
{
	return m_SupportedViewTypes;
}

void TerrainGBufferFillPass::SetupView(render::GeometryPassContext& abstractContext, nvrhi::ICommandList* commandList, const engine::IView* view, const engine::IView* viewPrev)
{
	auto& context = static_cast<Context&>(abstractContext);

	GBufferFillConstants gbufferConstants = {};
	view->FillPlanarViewConstants(gbufferConstants.view);
	viewPrev->FillPlanarViewConstants(gbufferConstants.viewPrev);
	commandList->writeBuffer(m_GBufferCB, &gbufferConstants, sizeof(gbufferConstants));

	context.keyTemplate.bits.frontCounterClockwise = view->IsMirrored();
	context.keyTemplate.bits.reverseDepth = view->IsReverseDepth();
}

bool TerrainGBufferFillPass::SetupMaterial(render::GeometryPassContext& abstractContext, const engine::Material* material, nvrhi::RasterCullMode cullMode, nvrhi::GraphicsState& state)
{
	auto& context = static_cast<Context&>(abstractContext);

	PipelineKey key = context.keyTemplate;
	key.bits.cullMode = cullMode;

	// Setup material bindings
	nvrhi::IBindingSet* materialBindingSet = m_MaterialBindings->GetMaterialBindingSet(material);

	if (!materialBindingSet)
		return false;

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
			return false;
		}
	}

	assert(pipeline->getFramebufferInfo() == state.framebuffer->getFramebufferInfo());

	state.pipeline = pipeline;
	state.bindings = {
		materialBindingSet,
		m_ViewBindingSet,
		context.inputBindingSet
	};

	return true;
}

void TerrainGBufferFillPass::SetupInputBuffers(render::GeometryPassContext& abstractContext, const engine::BufferGroup* buffers, nvrhi::GraphicsState& state)
{
	auto& context = static_cast<Context&>(abstractContext);

	// No index buffer is used for terrain
	state.setIndexBuffer({});

	nvrhi::BindingSetHandle inputBindingSet;
	auto it = m_InputBindingSets.find(buffers);
	if (it == m_InputBindingSets.end())
	{
		auto bindingSetDesc = nvrhi::BindingSetDesc()
			.addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(GBUFFER_BINDING_INSTANCE_BUFFER, buffers->instanceBuffer))
			.addItem(nvrhi::BindingSetItem::PushConstants(GBUFFER_BINDING_PUSH_CONSTANTS, sizeof(GBufferPushConstants)));

		inputBindingSet = m_Device->createBindingSet(bindingSetDesc, m_InputBindingLayout);
		m_InputBindingSets[buffers] = inputBindingSet;
	}
	else
	{
		inputBindingSet = it->second;
	}

	context.inputBindingSet = inputBindingSet;
}

void TerrainGBufferFillPass::SetPushConstants(render::GeometryPassContext& abstractContext, nvrhi::ICommandList* commandList, nvrhi::GraphicsState& state, nvrhi::DrawArguments& args)
{
	auto& context = static_cast<Context&>(abstractContext);

	GBufferPushConstants constants;
	constants.startInstanceLocation = args.startInstanceLocation;
	constants.startVertexLocation = args.startVertexLocation;
	constants.positionOffset = 0;
	constants.prevPositionOffset = 0;
	constants.texCoordOffset = 0;
	constants.normalOffset = 0;
	constants.tangentOffset = 0;

	commandList->setPushConstants(&constants, sizeof(constants));

	args.startInstanceLocation = 0;
	args.startVertexLocation = 0;
}

nvrhi::ShaderHandle TerrainGBufferFillPass::CreateVertexShader(engine::ShaderFactory& shaderFactory, const CreateParameters& params)
{
	char const* sourceFileName = "app/landscape_shaders.hlsl";

	return shaderFactory.CreateAutoShader(sourceFileName, "gbuffer_vs",
		DONUT_MAKE_PLATFORM_SHADER(g_landscape_shaders_gbuffer_vs), nullptr, nvrhi::ShaderType::Vertex);
}

nvrhi::ShaderHandle TerrainGBufferFillPass::CreatePixelShader(engine::ShaderFactory& shaderFactory, const CreateParameters& params)
{
	char const* sourceFileName = "donut/passes/gbuffer_ps.hlsl";

	std::vector<engine::ShaderMacro> PixelShaderMacros;
	PixelShaderMacros.emplace_back("MOTION_VECTORS", "0");
	PixelShaderMacros.emplace_back("ALPHA_TESTED", "0");

	return shaderFactory.CreateAutoShader(sourceFileName, "main", 
		DONUT_MAKE_PLATFORM_SHADER(g_landscape_shaders_gbuffer_ps), &PixelShaderMacros, nvrhi::ShaderType::Pixel);

}

std::shared_ptr<engine::MaterialBindingCache> TerrainGBufferFillPass::CreateMaterialBindingCache(engine::CommonRenderPasses& commonPasses)
{
	std::vector<engine::MaterialResourceBinding> materialBindings = {
		{ engine::MaterialResource::ConstantBuffer,         GBUFFER_BINDING_MATERIAL_CONSTANTS },
        { engine::MaterialResource::DiffuseTexture,         GBUFFER_BINDING_MATERIAL_DIFFUSE_TEXTURE },
        { engine::MaterialResource::SpecularTexture,        GBUFFER_BINDING_MATERIAL_SPECULAR_TEXTURE },
        { engine::MaterialResource::NormalTexture,          GBUFFER_BINDING_MATERIAL_NORMAL_TEXTURE },
        { engine::MaterialResource::EmissiveTexture,        GBUFFER_BINDING_MATERIAL_EMISSIVE_TEXTURE },
        { engine::MaterialResource::OcclusionTexture,       GBUFFER_BINDING_MATERIAL_OCCLUSION_TEXTURE },
        { engine::MaterialResource::TransmissionTexture,    GBUFFER_BINDING_MATERIAL_TRANSMISSION_TEXTURE },
        { engine::MaterialResource::OpacityTexture,         GBUFFER_BINDING_MATERIAL_OPACITY_TEXTURE }
	};

	return std::make_shared<engine::MaterialBindingCache>(
		m_Device,
		nvrhi::ShaderType::Pixel,
		GBUFFER_SPACE_MATERIAL,
		true,
		materialBindings,
		commonPasses.m_AnisotropicWrapSampler,
		commonPasses.m_GrayTexture);
}

nvrhi::BindingLayoutHandle TerrainGBufferFillPass::CreateInputBindingLayout(const CreateParameters& params)
{
	auto bindingLayoutDesc = nvrhi::BindingLayoutDesc()
		.setVisibility(nvrhi::ShaderType::Vertex | nvrhi::ShaderType::Pixel)
		.setRegisterSpace(GBUFFER_SPACE_INPUT)
		.setRegisterSpaceIsDescriptorSet(true)
		.addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(GBUFFER_BINDING_INSTANCE_BUFFER))
		.addItem(nvrhi::BindingLayoutItem::PushConstants(GBUFFER_BINDING_PUSH_CONSTANTS, sizeof(GBufferPushConstants)));

	return m_Device->createBindingLayout(bindingLayoutDesc);
}

void TerrainGBufferFillPass::CreateViewBindings(nvrhi::BindingLayoutHandle& layout, nvrhi::BindingSetHandle& set, const CreateParameters& params)
{
	auto bindingLayoutDesc = nvrhi::BindingLayoutDesc()
		.setVisibility(nvrhi::ShaderType::Vertex | nvrhi::ShaderType::Pixel)
		.setRegisterSpace(GBUFFER_SPACE_VIEW)
		.setRegisterSpaceIsDescriptorSet(true)
		.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(GBUFFER_BINDING_VIEW_CONSTANTS))
		.addItem(nvrhi::BindingLayoutItem::Sampler(GBUFFER_BINDING_MATERIAL_SAMPLER));

	layout = m_Device->createBindingLayout(bindingLayoutDesc);

	auto bindingSetDesc = nvrhi::BindingSetDesc()
		.setTrackLiveness(params.trackLiveness)
		.addItem(nvrhi::BindingSetItem::ConstantBuffer(GBUFFER_BINDING_VIEW_CONSTANTS, m_GBufferCB))
		.addItem(nvrhi::BindingSetItem::Sampler(GBUFFER_BINDING_MATERIAL_SAMPLER, m_CommonPasses->m_AnisotropicWrapSampler));

	set = m_Device->createBindingSet(bindingSetDesc, layout);
}

nvrhi::GraphicsPipelineHandle TerrainGBufferFillPass::CreateGraphicsPipeline(PipelineKey key, nvrhi::IFramebuffer* sampleFramebuffer)
{
	nvrhi::GraphicsPipelineDesc pipelineDesc;
	pipelineDesc.inputLayout = nullptr;
	pipelineDesc.VS = m_VertexShader;
	pipelineDesc.PS = m_PixelShader;
	pipelineDesc.bindingLayouts = {
		m_MaterialBindings->GetLayout(),
		m_ViewBindingLayout,
		m_InputBindingLayout
	};

	pipelineDesc.primType = nvrhi::PrimitiveType::TriangleStrip;

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



void landscapes::RenderTerrainView(
	nvrhi::ICommandList* commandList,
	const donut::engine::IView* view,
	const donut::engine::IView* viewPrev,
	nvrhi::IFramebuffer* framebuffer,
	donut::render::IDrawStrategy& drawStrategy,
	ITerrainPass& pass,
	donut::render::GeometryPassContext& passContext
)
{
	pass.SetupView(passContext, commandList, view, viewPrev);

	nvrhi::GraphicsState graphicsState;
	graphicsState.framebuffer = framebuffer;
	graphicsState.viewport = view->GetViewportState();
	graphicsState.shadingRateState = view->GetVariableRateShadingState();

	if (auto drawItem = drawStrategy.GetNextItem())
	{
		pass.SetupInputBuffers(passContext, drawItem->buffers, graphicsState);

		if (pass.SetupMaterial(passContext, drawItem->material, drawItem->cullMode, graphicsState))
		{
			commandList->setGraphicsState(graphicsState);

			nvrhi::DrawArguments args;
			args.vertexCount = drawItem->geometry->numVertices;
			args.instanceCount = 1;
			args.startVertexLocation = 0;
			args.startIndexLocation = 0;
			args.startInstanceLocation = 0;

			pass.SetPushConstants(passContext, commandList, graphicsState, args);

			commandList->draw(args);
		}
	}
}
