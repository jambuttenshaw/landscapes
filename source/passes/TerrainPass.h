#pragma once

#include <mutex>

#include <donut/render/GeometryPasses.h>
#include <donut/engine/CommonRenderPasses.h>

#include "terrain/Terrain.h"


struct TerrainPassContext
{
    bool wireframe = false;
};

class ITerrainPass
{
public:
    virtual ~ITerrainPass() = default;

    virtual void SetupView(TerrainPassContext& context, nvrhi::ICommandList* commandList, 
							const donut::engine::IView* view, const donut::engine::IView* viewPrev) = 0;
    virtual void SetupPipeline(TerrainPassContext& context, nvrhi::RasterCullMode cullMode, nvrhi::GraphicsState& state) = 0;
    virtual void SetupBindings(TerrainPassContext& context, const donut::engine::BufferGroup* buffers, 
								const TerrainMeshView* terrainView, nvrhi::GraphicsState& state) = 0;
};


class TerrainGBufferFillPass : public ITerrainPass
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

    struct Context : TerrainPassContext
	{
        friend TerrainGBufferFillPass;
        Context()
        {
            keyTemplate.value = 0;
        }
    private:

        PipelineKey keyTemplate;
    };

public:
    TerrainGBufferFillPass(nvrhi::IDevice* device, std::shared_ptr<donut::engine::CommonRenderPasses> commonPasses);
    virtual ~TerrainGBufferFillPass() = default;

    virtual void Init(donut::engine::ShaderFactory& shaderFactory);

    virtual void SetupView(TerrainPassContext& context, nvrhi::ICommandList* commandList, 
							const donut::engine::IView* view, const donut::engine::IView* viewPrev) override;
    virtual void SetupPipeline(TerrainPassContext& context, nvrhi::RasterCullMode cullMode, nvrhi::GraphicsState& state) override;
    void SetupBindings(TerrainPassContext& context, const donut::engine::BufferGroup* buffers,
						const TerrainMeshView* terrainView, nvrhi::GraphicsState& state) override;

protected:

    virtual nvrhi::ShaderHandle CreateVertexShader(donut::engine::ShaderFactory& shaderFactory);
    virtual nvrhi::ShaderHandle CreatePixelShader(donut::engine::ShaderFactory& shaderFactory);

    virtual nvrhi::BindingLayoutHandle CreateInputBindingLayout();
    virtual nvrhi::BindingLayoutHandle CreateTerrainBindingLayout();
    virtual void CreateViewBindings(nvrhi::BindingLayoutHandle& layout, nvrhi::BindingSetHandle& set);

	virtual nvrhi::GraphicsPipelineHandle CreateGraphicsPipeline(PipelineKey key, nvrhi::IFramebuffer* sampleFramebuffer);

    virtual nvrhi::BindingSetHandle CreateInputBindingSet(const donut::engine::BufferGroup* buffers);
    virtual nvrhi::BindingSetHandle CreateTerrainBindingSet(const TerrainMeshView* terrainView);

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

    // Sparse array of graphics pipelines
    std::array<nvrhi::GraphicsPipelineHandle, PipelineKey::Count> m_Pipelines{};
    std::mutex m_Mutex;

    std::unordered_map<const donut::engine::BufferGroup*, nvrhi::BindingSetHandle> m_InputBindingSets;
    std::unordered_map<const TerrainMeshView*, nvrhi::BindingSetHandle> m_TerrainBindingSets;
};


// Renders terrains for a given view
//  The draw strategy feeds the terrain instances
//  The terrain pass knows how to draw the terrain
//  The pass context contains data that the pass needs to cache for how to draw the terrain
void RenderTerrainView(
    nvrhi::ICommandList* commandList,
    const donut::engine::IView* view,
    const donut::engine::IView* viewPrev,
    nvrhi::IFramebuffer* framebuffer,
    const std::shared_ptr<donut::engine::SceneGraphNode>& rootNode,
    donut::render::IDrawStrategy& drawStrategy,
    ITerrainPass& pass,
    TerrainPassContext& passContext
);
