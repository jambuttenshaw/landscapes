#include "LandscapesApplication.h"

#include <nvrhi/utils.h>

#include <donut/render/DrawStrategy.h>

#include <donut/engine/ShaderFactory.h>
#include <donut/engine/CommonRenderPasses.h>
#include <donut/engine/FramebufferFactory.h>

#include "render/TerrainDrawStrategy.h"

using namespace donut;
using namespace donut::math;

const char* g_WindowTitle = "Landscapes";


LandscapesApplication::LandscapesApplication(donut::app::DeviceManager* deviceManager, UIData& ui)
	: ApplicationBase(deviceManager)
	, m_UI(ui)
{
    SetAsynchronousLoadingEnabled(false);

    m_NativeFS = std::make_shared<vfs::NativeFileSystem>();

    std::filesystem::path appShaderPath = app::GetDirectoryWithExecutable() / "shaders/landscapes" / app::GetShaderTypeName(GetDevice()->getGraphicsAPI());
    std::filesystem::path frameworkShaderPath = app::GetDirectoryWithExecutable() / "shaders/framework" / app::GetShaderTypeName(GetDevice()->getGraphicsAPI());

    std::shared_ptr<vfs::RootFileSystem> rootFS = std::make_shared<vfs::RootFileSystem>();
    rootFS->mount("/shaders/donut", frameworkShaderPath);
    rootFS->mount("/shaders/app", appShaderPath);

    m_ShaderFactory = std::make_shared<engine::ShaderFactory>(GetDevice(), rootFS, "/shaders");
    m_CommonPasses = std::make_shared<engine::CommonRenderPasses>(GetDevice(), m_ShaderFactory);
    m_TextureCache = std::make_shared<engine::TextureCache>(GetDevice(), m_NativeFS, nullptr);
    m_BindingCache = std::make_unique<engine::BindingCache>(GetDevice());

    m_DeferredLightingPass = std::make_unique<render::DeferredLightingPass>(GetDevice(), m_CommonPasses);
    m_DeferredLightingPass->Init(m_ShaderFactory);

    m_GBufferVisualizationPass = std::make_unique<GBufferVisualizationPass>(GetDevice());
    m_GBufferVisualizationPass->Init(m_ShaderFactory);

    m_DebugPlanePass = std::make_unique<DebugPlanePass>(GetDevice());
    m_DebugPlanePass->Init(m_ShaderFactory);

    m_TerrainTessellator = std::make_unique<TerrainTessellator>(GetDevice());
    m_TerrainTessellator->Init(*m_ShaderFactory);

    m_CommandList = GetDevice()->createCommandList();

    m_Camera.LookAt(float3{ 0.0f, 250.0f, 0.0f }, float3{ 0.0f, 0.f, 0.0f }, float3{ 0.0f, 0.0f, 1.0f });
    m_Camera.SetMoveSpeed(75.0f);

    // TODO: Avoid calling virtual function in constructor
    BeginLoadingScene(m_NativeFS, app::GetDirectoryWithExecutable().parent_path() / "media/landscapes.scene.json");
}

void LandscapesApplication::CreateDeferredShadingOutput(nvrhi::IDevice* device, dm::uint2 size, dm::uint sampleCount)
{
    nvrhi::TextureDesc textureDesc;
    textureDesc.dimension = nvrhi::TextureDimension::Texture2D;
    textureDesc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    textureDesc.keepInitialState = true;
    textureDesc.debugName = "ShadedColor";
    textureDesc.isUAV = true;
    textureDesc.format = nvrhi::Format::RGBA16_FLOAT;
    textureDesc.width = size.x;
    textureDesc.height = size.y;
    textureDesc.sampleCount = sampleCount;
    m_ShadedColour = device->createTexture(textureDesc);
}

void LandscapesApplication::CreateGBufferPasses()
{
    render::GBufferFillPass::CreateParameters GBufferParams;
    GBufferParams.useInputAssembler = false;

    m_GBufferPass = std::make_unique<render::GBufferFillPass>(GetDevice(), m_CommonPasses);
    m_GBufferPass->Init(*m_ShaderFactory, GBufferParams);

    m_TerrainGBufferPass = std::make_unique<TerrainGBufferFillPass>(GetDevice(), m_CommonPasses);
    m_TerrainGBufferPass->Init(*m_ShaderFactory);
}

void LandscapesApplication::SceneUnloading()
{
	// Reset binding caches
    // Clear any references to objects in the scene
}

bool LandscapesApplication::LoadScene(std::shared_ptr<vfs::IFileSystem> fs, const std::filesystem::path& sceneFileName)
{
    std::unique_ptr<LandscapesScene> scene = std::make_unique<LandscapesScene>(
		m_UI,
		GetDevice(),
        *m_ShaderFactory, 
        fs, 
        m_TextureCache, 
        nullptr, 
        nullptr,
        m_CommonPasses);

	if (scene->Load(sceneFileName))
    {
	    m_Scene = std::move(scene);
        return true;
    }

	return false;
}

void LandscapesApplication::SceneLoaded()
{
	ApplicationBase::SceneLoaded();

    m_Scene->FinishedLoading(GetFrameIndex());

    // Set up lights, camera, etc...

    for (auto light : m_Scene->GetSceneGraph()->GetLights())
    {
        if (light->GetLightType() == LightType_Directional)
        {
            m_SunLight = std::static_pointer_cast<donut::engine::DirectionalLight>(light);
            if (m_SunLight->irradiance <= 0.f)
                m_SunLight->irradiance = 1.f;
            break;
        }
    }

    if (!m_SunLight)
    {
        m_SunLight = std::make_shared<donut::engine::DirectionalLight>();
        m_SunLight->angularSize = 0.53f;
        m_SunLight->irradiance = 1.f;

        auto node = std::make_shared<donut::engine::SceneGraphNode>();
        node->SetLeaf(m_SunLight);
        m_SunLight->SetDirection(dm::double3(0.1, -0.9, 0.1));
        m_SunLight->SetName("Sun");
        m_Scene->GetSceneGraph()->Attach(m_Scene->GetSceneGraph()->GetRootNode(), node);
    }
}

bool LandscapesApplication::KeyboardUpdate(int key, int scancode, int action, int mods)
{
    m_Camera.KeyboardUpdate(key, scancode, action, mods);
	return true;
}

bool LandscapesApplication::MousePosUpdate(double xpos, double ypos)
{
    m_Camera.MousePosUpdate(xpos, ypos);
	return true;
}

bool LandscapesApplication::MouseButtonUpdate(int button, int action, int mods)
{
    m_Camera.MouseButtonUpdate(button, action, mods);
	return true;
}

void LandscapesApplication::Animate(float fElapsedTimeSeconds)
{
    m_Camera.Animate(fElapsedTimeSeconds);
	GetDeviceManager()->SetInformativeWindowTitle(g_WindowTitle);

    m_UI.CameraPosition = m_Camera.GetPosition();

    if (m_SunLight)
    {
		m_SunLight->SetDirection(static_cast<dm::double3>(m_UI.LightDirection));
    }
}

void LandscapesApplication::BackBufferResizing()
{

}

void LandscapesApplication::RenderScene(nvrhi::IFramebuffer* framebuffer)
{
	if (!m_Scene)
	{
        // Nothing to render
		return;
	}

    const auto& fbInfo = framebuffer->getFramebufferInfo();

    nvrhi::Viewport windowViewport(static_cast<float>(fbInfo.width), static_cast<float>(fbInfo.height));
    m_View.SetViewport(windowViewport);
    m_View.SetMatrices(
        m_Camera.GetWorldToViewMatrix(),
        perspProjD3DStyleReverse(dm::PI_f * 0.25f, windowViewport.width() / windowViewport.height(), 0.1f)
    );
    m_View.UpdateCache();

    uint2 size = uint2(fbInfo.width, fbInfo.height);
    if ((!m_GBuffer || any(m_GBuffer->GetSize() != size)))
    {
	    // m_ShadedColour should always match GBuffer
        m_GBuffer = nullptr;
        m_ShadedColour = nullptr;
        m_BindingCache->Clear();
        m_DeferredLightingPass->ResetBindingCache();
        m_GBufferVisualizationPass->ResetBindingCache();
        m_DebugPlanePass->ResetPipeline();

        m_GBufferPass.reset();

        m_GBuffer = std::make_shared<render::GBufferRenderTargets>();
        m_GBuffer->Init(GetDevice(), size, 1, false, m_View.IsReverseDepth());
        CreateDeferredShadingOutput(GetDevice(), size, 1);
    }

    if (!m_GBufferPass)
    {
        CreateGBufferPasses();
    }

    m_CommandList->open();

    m_Scene->Refresh(m_CommandList, GetFrameIndex());

    m_GBuffer->Clear(m_CommandList);

    // Update terrain
    if (m_UI.UpdateTerrain)
	{
        TerrainDrawStrategy drawStrategy;
        TessellateTerrainView(
			m_CommandList,
            &m_View,
            m_Scene->GetSceneGraph()->GetRootNode(),
            drawStrategy,
            *m_TerrainTessellator
        );
    }

    // Draw terrain
    if (m_UI.DrawTerrain)
    {
        TerrainDrawStrategy drawStrategy;
        TerrainGBufferFillPass::Context context;
        context.wireframe = m_UI.Wireframe;

        RenderTerrainView(
            m_CommandList,
            &m_View,
            &m_View,
            m_GBuffer->GBufferFramebuffer->GetFramebuffer(m_View),
            m_Scene->GetSceneGraph()->GetRootNode(),
            drawStrategy,
            *m_TerrainGBufferPass,
            context
        );
    }

	{
		donut::render::InstancedOpaqueDrawStrategy drawStrategy;
        donut::render::GBufferFillPass::Context context;

        drawStrategy.PrepareForView(m_Scene->GetSceneGraph()->GetRootNode(), m_View);

		render::RenderView(
			m_CommandList,
            &m_View,
            &m_View,
            m_GBuffer->GBufferFramebuffer->GetFramebuffer(m_View),
            drawStrategy,
            *m_GBufferPass,
            context
        );
	}

    /*
    if (m_UI.ShowDebugPlane)
    {
        m_DebugPlanePass->Render(
	    	m_CommandList, 
            m_View, 
            m_GBuffer->GBufferFramebuffer->GetFramebuffer(m_View),
            m_UI.DebugPlaneNormal,
            m_UI.DebugPlaneOrigin
        );
    }
	*/

    render::DeferredLightingPass::Inputs deferredInputs;
    deferredInputs.SetGBuffer(*m_GBuffer);
    deferredInputs.ambientColorTop = 0.0f;
    deferredInputs.ambientColorBottom = deferredInputs.ambientColorTop * float3(0.3f, 0.4f, 0.3f);
    deferredInputs.lights = &m_Scene->GetSceneGraph()->GetLights();
    deferredInputs.output = m_ShadedColour;

    m_DeferredLightingPass->Render(m_CommandList, m_View, deferredInputs);

    // Debug view modes
    switch(m_UI.ViewMode)
    {
	case ViewModes::Unlit:
		m_GBufferVisualizationPass->Render(m_CommandList, m_View, *m_GBuffer, GBufferVisualizationPass::VisualizationMode_Unlit, m_ShadedColour);
        break;
	case ViewModes::Normals:
		m_GBufferVisualizationPass->Render(m_CommandList, m_View, *m_GBuffer, GBufferVisualizationPass::VisualizationMode_Normals, m_ShadedColour);
        break;
    default:
        break;
    }

    m_CommonPasses->BlitTexture(m_CommandList, framebuffer, m_ShadedColour, m_BindingCache.get());
    
    m_CommandList->close();
    GetDevice()->executeCommandList(m_CommandList);
}
