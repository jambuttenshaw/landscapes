#pragma once

#include <functional>
#include <mutex>

#include <donut/render/GeometryPasses.h>
#include <donut/engine/CommonRenderPasses.h>
#include <donut/engine/MaterialBindingCache.h>


class Terrain;

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
    TerrainGBufferFillPass(nvrhi::IDevice* device, std::shared_ptr<donut::engine::CommonRenderPasses> commonPasses);
    virtual ~TerrainGBufferFillPass() = default;

    virtual void Init(donut::engine::ShaderFactory& shaderFactory);

    void RenderTerrain(
        nvrhi::ICommandList* commandList,
        const donut::engine::IView* view,
        const donut::engine::IView* viewPrev,
        nvrhi::IFramebuffer* framebuffer,
        const Terrain* terrain,
        bool wireframe,
        donut::render::IDrawStrategy& drawStrategy
    );

protected:

    virtual nvrhi::ShaderHandle CreateVertexShader(donut::engine::ShaderFactory& shaderFactory);
    virtual nvrhi::ShaderHandle CreatePixelShader(donut::engine::ShaderFactory& shaderFactory);

    virtual nvrhi::BindingLayoutHandle CreateInputBindingLayout();
    virtual nvrhi::BindingLayoutHandle CreateTerrainBindingLayout();
    virtual void CreateViewBindings(nvrhi::BindingLayoutHandle& layout, nvrhi::BindingSetHandle& set);

	virtual nvrhi::GraphicsPipelineHandle CreateGraphicsPipeline(PipelineKey key, nvrhi::IFramebuffer* sampleFramebuffer);

    virtual nvrhi::BindingSetHandle CreateInputBindingSet(const donut::engine::BufferGroup* buffers);
    virtual nvrhi::BindingSetHandle CreateTerrainBindingSet(const Terrain* terrain);

protected:
    nvrhi::DeviceHandle m_Device;
    std::shared_ptr<donut::engine::CommonRenderPasses> m_CommonPasses;

    nvrhi::ShaderHandle m_VertexShader;
    nvrhi::ShaderHandle m_PixelShader;

    nvrhi::BindingLayoutHandle m_InputBindingLayout;
    nvrhi::BindingLayoutHandle m_TerrainBindingLayout;

    nvrhi::BindingLayoutHandle m_ViewBindingLayout;
    nvrhi::BindingSetHandle m_ViewBindingSet;

    nvrhi::BufferHandle m_GBufferCB;
    nvrhi::BufferHandle m_TerrainCB;

    // Sparse array of graphics pipelines
    nvrhi::GraphicsPipelineHandle m_Pipelines[PipelineKey::Count];
    std::mutex m_Mutex;

    std::unordered_map<const donut::engine::BufferGroup*, nvrhi::BindingSetHandle> m_InputBindingSets;
    std::unordered_map<const Terrain*, nvrhi::BindingSetHandle> m_TerrainBindingSets;
};
