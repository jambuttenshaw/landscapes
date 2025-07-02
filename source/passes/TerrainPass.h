#pragma once

#include <mutex>

#include <donut/render/GeometryPasses.h>
#include <donut/engine/CommonRenderPasses.h>
#include <donut/engine/MaterialBindingCache.h>


class TerrainGBufferFillPass
{
public:
    union PipelineKey
    {
	    struct
	    {
            nvrhi::RasterCullMode cullMode : 2;
            bool frontCounterClockwise : 1;
            bool wireframe : 1;
            bool reverseDepth : 1;
	    } bits;
        uint32_t value;

        static constexpr size_t Count = 1 << 5;
    };

public:
    TerrainGBufferFillPass(nvrhi::IDevice* device);
    virtual ~TerrainGBufferFillPass() = default;

    virtual void Init(donut::engine::ShaderFactory& shaderFactory);

    void RenderTerrain(
        nvrhi::ICommandList* commandList,
        const donut::engine::IView* view,
        const donut::engine::IView* viewPrev,
        nvrhi::IFramebuffer* framebuffer,
        bool wireframe,
        donut::render::IDrawStrategy& drawStrategy // contains the terrain item to draw
    );

protected:

    virtual nvrhi::ShaderHandle CreateVertexShader(donut::engine::ShaderFactory& shaderFactory);
    virtual nvrhi::ShaderHandle CreatePixelShader(donut::engine::ShaderFactory& shaderFactory);

    virtual nvrhi::BindingLayoutHandle CreateInputBindingLayout();
    virtual void CreateViewBindings(nvrhi::BindingLayoutHandle& layout, nvrhi::BindingSetHandle& set);

	virtual nvrhi::GraphicsPipelineHandle CreateGraphicsPipeline(PipelineKey key, nvrhi::IFramebuffer* sampleFramebuffer);

protected:
    nvrhi::DeviceHandle m_Device;

    nvrhi::ShaderHandle m_VertexShader;
    nvrhi::ShaderHandle m_PixelShader;

    nvrhi::BindingLayoutHandle m_InputBindingLayout;
    nvrhi::BindingLayoutHandle m_ViewBindingLayout;
    nvrhi::BindingSetHandle m_ViewBindingSet;

    nvrhi::BufferHandle m_GBufferCB;

    // Sparse array of graphics pipelines
    nvrhi::GraphicsPipelineHandle m_Pipelines[PipelineKey::Count];
    std::mutex m_Mutex;

    std::unordered_map<const donut::engine::BufferGroup*, nvrhi::BindingSetHandle> m_InputBindingSets;
};
