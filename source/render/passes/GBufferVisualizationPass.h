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


class GBufferVisualizationPass
{
public:

    enum VisualizationMode : int8_t
    {
        VisualizationMode_None = -1,

	    VisualizationMode_Unlit,
        VisualizationMode_Normals,

        VisualizationMode_COUNT
    };

public:
	GBufferVisualizationPass(nvrhi::IDevice* device);
    virtual ~GBufferVisualizationPass() = default;

	void Init(const std::shared_ptr<donut::engine::ShaderFactory>& shaderFactory);

    void Render(
        nvrhi::ICommandList* commandList,
        const donut::engine::IView& view,
        const donut::render::GBufferRenderTargets& gbuffer,
        VisualizationMode mode,
        nvrhi::ITexture* output);

    void ResetBindingCache();

private:
    nvrhi::DeviceHandle m_Device;

    std::array<nvrhi::ShaderHandle, VisualizationMode_COUNT> m_Shaders;
    std::array<nvrhi::ComputePipelineHandle, VisualizationMode_COUNT> m_Pipelines;

    nvrhi::BindingLayoutHandle m_BindingLayout;
    donut::engine::BindingCache m_BindingSets;
};
