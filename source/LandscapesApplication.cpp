#include "LandscapesApplication.h"

#include <donut/engine/ShaderFactory.h>
#include <nvrhi/utils.h>

#include <donut/render/DrawStrategy.h>
#include <donut/render/GeometryPasses.h>
#include <donut/engine/CommonRenderPasses.h>

#include "donut/engine/FramebufferFactory.h"
#include "passes/TerrainDrawStrategy.h"

using namespace donut;
using namespace donut::math;

const char* g_WindowTitle = "Landscapes";


LandscapesApplication::LandscapesApplication(donut::app::DeviceManager* deviceManager, UIData& ui)
	: ApplicationBase(deviceManager)
	, m_UI(ui)
{
}

bool LandscapesApplication::Init()
{
    auto nativeFS = std::make_shared<vfs::NativeFileSystem>();

    std::filesystem::path appShaderPath = app::GetDirectoryWithExecutable() / "shaders/landscapes" / app::GetShaderTypeName(GetDevice()->getGraphicsAPI());
    std::filesystem::path frameworkShaderPath = app::GetDirectoryWithExecutable() / "shaders/framework" / app::GetShaderTypeName(GetDevice()->getGraphicsAPI());

    std::shared_ptr<vfs::RootFileSystem> rootFS = std::make_shared<vfs::RootFileSystem>();
    rootFS->mount("/shaders/donut", frameworkShaderPath);
    rootFS->mount("/shaders/app", appShaderPath);

    m_ShaderFactory = std::make_shared<engine::ShaderFactory>(GetDevice(), rootFS, "/shaders");
    m_CommonPasses = std::make_shared<engine::CommonRenderPasses>(GetDevice(), m_ShaderFactory);
    m_TextureCache = std::make_shared<engine::TextureCache>(GetDevice(), nativeFS, nullptr);
    m_BindingCache = std::make_unique<engine::BindingCache>(GetDevice());

    m_DeferredLightingPass = std::make_unique<render::DeferredLightingPass>(GetDevice(), m_CommonPasses);
    m_DeferredLightingPass->Init(m_ShaderFactory);

    m_CommandList = GetDevice()->createCommandList();

    m_Camera.LookAt(float3{ 0.0f, 15.0f, -50.0f }, float3{ 0.0f, 10.0f, 0.0f });

    m_CommandList->open();

    bool success = m_Scene.Init(GetDevice(), m_CommandList, m_TextureCache.get());

    m_CommandList->close();
    GetDevice()->executeCommandList(m_CommandList);

	return success;
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

    m_TerrainGBufferPass = std::make_unique<TerrainGBufferFillPass>(GetDevice());
    m_TerrainGBufferPass->Init(*m_ShaderFactory);
}

bool LandscapesApplication::LoadScene(std::shared_ptr<vfs::IFileSystem> fs, const std::filesystem::path& sceneFileName)
{
	return true;
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
}

void LandscapesApplication::BackBufferResizing()
{

}

void LandscapesApplication::Render(nvrhi::IFramebuffer* framebuffer)
{
    const auto& fbInfo = framebuffer->getFramebufferInfo();

    uint2 size = uint2(fbInfo.width, fbInfo.height);
    if ((!m_GBuffer || any(m_GBuffer->GetSize() != size)))
    {
	    // m_ShadedColour should always match GBuffer
        m_GBuffer = nullptr;
        m_ShadedColour = nullptr;
        m_BindingCache->Clear();
        m_DeferredLightingPass->ResetBindingCache();

        m_GBufferPass.reset();

        m_GBuffer = std::make_shared<render::GBufferRenderTargets>();
        m_GBuffer->Init(GetDevice(), size, 1, false, true);
        CreateDeferredShadingOutput(GetDevice(), size, 1);
    }

    nvrhi::Viewport windowViewport(static_cast<float>(fbInfo.width), static_cast<float>(fbInfo.height));
    m_View.SetViewport(windowViewport);
    m_View.SetMatrices(
        m_Camera.GetWorldToViewMatrix(), 
        perspProjD3DStyleReverse(dm::PI_f * 0.25f, windowViewport.width() / windowViewport.height(), 0.1f)
    );
    m_View.UpdateCache();

    if (!m_GBufferPass)
    {
        CreateGBufferPasses();
    }

    m_CommandList->open();

    m_GBuffer->Clear(m_CommandList);

    // Draw terrain
    if (m_UI.DrawTerrain)
    {
        Terrain* terrain = m_Scene.GetTerrain().get();

        TerrainDrawStrategy drawStrategy;
        uint lod = clamp(static_cast<uint>(m_UI.TerrainLOD), 0u, terrain->GetNumLevels() - 1);
        drawStrategy.SetData(terrain, lod);

        m_TerrainGBufferPass->RenderTerrain(
            m_CommandList,
            &m_View,
            &m_View,
            m_GBuffer->GBufferFramebuffer->GetFramebuffer(m_View),
            m_UI.Wireframe, // TODO: This was quite nice before passing extra state through as a context
            drawStrategy
        );
    }

    // Draw objects in scene
    if (m_UI.DrawObjects)
    {
        render::DrawItem drawItem;
        drawItem.instance = m_Scene.GetCubeMeshInstance().get();
        drawItem.mesh = drawItem.instance->GetMesh().get();
        drawItem.geometry = drawItem.mesh->geometries[0].get();
        drawItem.material = drawItem.geometry->material.get();
        drawItem.buffers = drawItem.mesh->buffers.get();
        drawItem.distanceToCamera = 0;
        drawItem.cullMode = GetCullMode();

        render::PassthroughDrawStrategy drawStrategy;
        drawStrategy.SetData(&drawItem, 1);

        render::GBufferFillPass::Context context;

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

    render::DeferredLightingPass::Inputs deferredInputs;
    deferredInputs.SetGBuffer(*m_GBuffer);
    deferredInputs.ambientColorTop = 0.2f;
    deferredInputs.ambientColorBottom = deferredInputs.ambientColorTop * float3(0.3f, 0.4f, 0.3f);
    deferredInputs.lights = &m_Scene.GetLights();
    deferredInputs.output = m_ShadedColour;

    m_DeferredLightingPass->Render(m_CommandList, m_View, deferredInputs);

    m_CommonPasses->BlitTexture(m_CommandList, framebuffer, m_ShadedColour, m_BindingCache.get());
    
    m_CommandList->close();
    GetDevice()->executeCommandList(m_CommandList);
}
