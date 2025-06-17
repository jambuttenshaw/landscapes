#include "landscapes_application.h"

#include <donut/engine/ShaderFactory.h>
#include <nvrhi/utils.h>

#include <donut/engine/CommonRenderPasses.h>

using namespace donut;
using namespace donut::math;

#include <donut/shaders/view_cb.h>

const char* g_WindowTitle = "Landscapes";


bool LandscapesApplication::Init()
{
    std::filesystem::path appShaderPath = app::GetDirectoryWithExecutable() / "shaders/landscapes" / app::GetShaderTypeName(GetDevice()->getGraphicsAPI());

    auto nativeFS = std::make_shared<vfs::NativeFileSystem>();
    engine::ShaderFactory shaderFactory(GetDevice(), nativeFS, appShaderPath);

    m_VertexShader = shaderFactory.CreateShader("shaders.hlsl", "main_vs", nullptr, nvrhi::ShaderType::Vertex);
    m_PixelShader = shaderFactory.CreateShader("shaders.hlsl", "main_ps", nullptr, nvrhi::ShaderType::Pixel);

    if (!m_VertexShader || !m_PixelShader)
    {
        return false;
    }

    m_CommandList = GetDevice()->createCommandList();

    m_Camera.LookAt(float3(0.f, 0.f, 2.f), float3(0.f, 0.f, -1.f));

    m_ViewConstants = GetDevice()->createBuffer(
        nvrhi::utils::CreateVolatileConstantBufferDesc(sizeof(PlanarViewConstants), "ViewConstants", engine::c_MaxRenderPassConstantBufferVersions)
    );

    // Create binding set / binding layout
    nvrhi::BindingSetDesc bindingSetDesc;
    bindingSetDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(0, m_ViewConstants)
    };
    nvrhi::utils::CreateBindingSetAndLayout(
        GetDevice(), nvrhi::ShaderType::All, 0, bindingSetDesc, m_BindingLayout, m_BindingSet
    );

	return true;
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
    m_Pipeline = nullptr;
}

void LandscapesApplication::Render(nvrhi::IFramebuffer* framebuffer)
{
    const auto& fbInfo = framebuffer->getFramebufferInfo();

    if (!m_Pipeline)
    {
        nvrhi::GraphicsPipelineDesc psoDesc;
        psoDesc.VS = m_VertexShader;
        psoDesc.PS = m_PixelShader;
        psoDesc.primType = nvrhi::PrimitiveType::TriangleStrip;
        psoDesc.bindingLayouts = { m_BindingLayout };
        psoDesc.renderState.depthStencilState.depthTestEnable = false;

        m_Pipeline = GetDevice()->createGraphicsPipeline(psoDesc, framebuffer);
    }

    nvrhi::Viewport windowViewport(static_cast<float>(fbInfo.width), static_cast<float>(fbInfo.height));
    m_View.SetViewport(windowViewport);
    m_View.SetMatrices(m_Camera.GetWorldToViewMatrix(), perspProjD3DStyleReverse(dm::PI_f * 0.25f, windowViewport.width() / windowViewport.height(), 0.1f));
    m_View.UpdateCache();

    m_CommandList->open();

    PlanarViewConstants viewConstants;
    m_View.FillPlanarViewConstants(viewConstants);
    m_CommandList->writeBuffer(m_ViewConstants, &viewConstants, sizeof(viewConstants));

    nvrhi::utils::ClearColorAttachment(m_CommandList, framebuffer, 0, nvrhi::Color(0.f));

    nvrhi::GraphicsState state;
    state.pipeline = m_Pipeline;
    state.framebuffer = framebuffer;
    state.bindings = { m_BindingSet };
    state.viewport = m_View.GetViewportState();

    m_CommandList->setGraphicsState(state);

    nvrhi::DrawArguments args;
    args.vertexCount = 4;
    m_CommandList->draw(args);

    m_CommandList->close();
    GetDevice()->executeCommandList(m_CommandList);
}
