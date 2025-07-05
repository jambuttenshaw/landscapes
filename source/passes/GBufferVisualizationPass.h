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
	GBufferVisualizationPass(nvrhi::IDevice* device, std::shared_ptr<donut::engine::CommonRenderPasses> commonPasses);
    virtual ~GBufferVisualizationPass() = default;

	void Init(const std::shared_ptr<donut::engine::ShaderFactory>& shaderFactory);

    void Render(
        nvrhi::ICommandList* commandList,
        const donut::engine::IView& view,
        const donut::render::GBufferRenderTargets& gbuffer,
        nvrhi::ITexture* output);

    void ResetBindingCache();

protected:

    virtual nvrhi::ShaderHandle CreateComputeShader(donut::engine::ShaderFactory& shaderFactory);

private:
    nvrhi::DeviceHandle m_Device;

    nvrhi::ShaderHandle m_ComputeShader;
    nvrhi::ComputePipelineHandle m_Pso;

    nvrhi::BindingLayoutHandle m_BindingLayout;
    donut::engine::BindingCache m_BindingSets;

    std::shared_ptr<donut::engine::CommonRenderPasses> m_CommonPasses;
};
