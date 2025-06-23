#pragma once

#include <mutex>

#include <donut/render/GeometryPasses.h>
#include <donut/engine/CommonRenderPasses.h>
#include <donut/engine/MaterialBindingCache.h>
#include <donut/engine/ShaderFactory.h>


// A terrain pass is just a geometry pass - the difference is in how the geometry is described and drawn
class ITerrainPass : public donut::render::IGeometryPass
{
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
            bool reverseDepth : 1;
	    } bits;
        uint32_t value;

        static constexpr size_t Count = 1 << 3;
        static_assert(8 * sizeof(bits) <= Count);
    };

    struct Context : public donut::render::GeometryPassContext
    {
        nvrhi::BindingSetHandle inputBindingSet;
        PipelineKey keyTemplate;

        Context()
        {
            keyTemplate.value = 0;
        }
    };

    struct CreateParameters
    {
        std::shared_ptr<donut::engine::MaterialBindingCache> materialBindings;
        bool trackLiveness = true;

        uint32_t numConstantBufferVersions = 16;
    };

public:
    TerrainGBufferFillPass(nvrhi::IDevice* device, std::shared_ptr<donut::engine::CommonRenderPasses> commonPasses);

    virtual void Init(donut::engine::ShaderFactory& shaderFactory, const CreateParameters& params);

    // IGeometryPass Implementation
    [[nodiscard]] donut::engine::ViewType::Enum GetSupportedViewTypes() const override;
    void SetupView(donut::render::GeometryPassContext& context, nvrhi::ICommandList* commandList, const donut::engine::IView* view, const donut::engine::IView* viewPrev) override;
    bool SetupMaterial(donut::render::GeometryPassContext& context, const donut::engine::Material* material, nvrhi::RasterCullMode cullMode, nvrhi::GraphicsState& state) override;
    void SetupInputBuffers(donut::render::GeometryPassContext& context, const donut::engine::BufferGroup* buffers, nvrhi::GraphicsState& state) override;
    void SetPushConstants(donut::render::GeometryPassContext& context, nvrhi::ICommandList* commandList, nvrhi::GraphicsState& state, nvrhi::DrawArguments& args) override;

protected:

    virtual nvrhi::ShaderHandle CreateVertexShader(donut::engine::ShaderFactory& shaderFactory, const CreateParameters& params);
    virtual nvrhi::ShaderHandle CreatePixelShader(donut::engine::ShaderFactory& shaderFactory, const CreateParameters& params);

    virtual std::shared_ptr<donut::engine::MaterialBindingCache> CreateMaterialBindingCache(donut::engine::CommonRenderPasses& commonPasses);
    virtual nvrhi::BindingLayoutHandle CreateInputBindingLayout(const CreateParameters& params);
    virtual void CreateViewBindings(nvrhi::BindingLayoutHandle& layout, nvrhi::BindingSetHandle& set, const CreateParameters& params);

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

    donut::engine::ViewType::Enum m_SupportedViewTypes;

    std::shared_ptr<donut::engine::CommonRenderPasses> m_CommonPasses;
    std::shared_ptr<donut::engine::MaterialBindingCache> m_MaterialBindings;
};


namespace landscapes
{
	
void RenderTerrainView(
    nvrhi::ICommandList* commandList,
    const donut::engine::IView* view,
    const donut::engine::IView* viewPrev,
    nvrhi::IFramebuffer* framebuffer,
    donut::render::IDrawStrategy& drawStrategy,
    ITerrainPass& pass,
    donut::render::GeometryPassContext& passContext
);

}
