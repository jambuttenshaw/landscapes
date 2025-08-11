#pragma once

#include <nvrhi/nvrhi.h>

#include <donut/engine/CommonRenderPasses.h>
#include <donut/engine/BindingCache.h>


namespace donut::render
{
    class GBufferRenderTargets;
}

namespace donut::engine
{
    class IView;
}


class DebugPlanePass
{
public:
    DebugPlanePass(nvrhi::DeviceHandle device)
	    : m_Device(std::move(device))
	{}

    void Init(const std::shared_ptr<donut::engine::ShaderFactory>& shaderFactory);

    void Render(
        nvrhi::ICommandList* commandList,
        const donut::engine::IView& view,
        nvrhi::IFramebuffer* framebuffer,
        donut::math::float3 PlaneNormal,
        donut::math::float3 PlaneOrigin
    );

    void ResetPipeline() { m_Pipeline = nullptr; }

private:
    nvrhi::DeviceHandle m_Device;

    nvrhi::ShaderHandle m_VertexShader;
    nvrhi::ShaderHandle m_PixelShader;
    nvrhi::GraphicsPipelineHandle m_Pipeline;

    nvrhi::BufferHandle m_ConstantBuffer;

    nvrhi::BindingLayoutHandle m_BindingLayout;
    nvrhi::BindingSetHandle m_BindingSet;
};
