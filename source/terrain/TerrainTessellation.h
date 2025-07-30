#pragma once

#include <donut/engine/ShaderFactory.h>
#include <donut/engine/View.h>


class TerrainMeshView;


class ITerrainTessellationPass
{
public:
    ITerrainTessellationPass(nvrhi::DeviceHandle device);
    virtual ~ITerrainTessellationPass() = default;

    virtual void Init(donut::engine::ShaderFactory& shaderFactory) = 0;

    virtual void SetupView(const donut::engine::IView* view) = 0;
    virtual void SetupSplitState(const TerrainMeshView* terrainView, nvrhi::ComputeState& state) = 0;
    virtual void SetupMergeState(const TerrainMeshView* terrainView, nvrhi::ComputeState& state) = 0;
    virtual void SetupPushConstants(nvrhi::ICommandList* commandList) = 0;

protected:
    nvrhi::DeviceHandle m_Device;
};

class TerrainTessellator
{
public:
    TerrainTessellator(nvrhi::DeviceHandle device);

    void Init(donut::engine::ShaderFactory& shaderFactory);

    void ExecutePassForTerrainView(
        nvrhi::ICommandList* commandList,
        const donut::engine::IView* view,
        ITerrainTessellationPass& pass,
        const TerrainMeshView* terrainView
    );

protected:
    nvrhi::DeviceHandle m_Device;

    // Shader handles
    // 1-to-1 relationship between shaders and pipelines here, so re-using the same enum
    enum Shaders : uint8_t
    {
        Shaders_CBTDispatch = 0,
        Shaders_LEBDispatch,
        Shaders_SumReductionPrePass,
        Shaders_SumReduction,

        Shaders_Count
    };
    std::array<nvrhi::ShaderHandle, Shaders_Count> m_Shaders{};
    std::array<nvrhi::ComputePipelineHandle, Shaders_Count> m_Pipelines{};

    enum Bindings : uint8_t
    {
	    Bindings_CBTReadOnly = 0, // For dispatchers
        Bindings_CBTReadWrite,    // For reduction sum pipelines
        Bindings_Count
    };
    std::array<nvrhi::BindingLayoutHandle, Bindings_Count> m_BindingLayouts{};

    // The tessellator requires some persistent state for each terrain mesh view
    struct TerrainCachedData
    {
        bool split = true; // flip-flops between slipping and merging
        std::array<nvrhi::BindingSetHandle, Bindings_Count> bindings;
    };
    std::unordered_map<const TerrainMeshView*, TerrainCachedData> m_TerrainCache;
};


class PrimaryViewTerrainTessellationPass : public ITerrainTessellationPass
{
public:
    PrimaryViewTerrainTessellationPass(nvrhi::DeviceHandle device);

    void Init(donut::engine::ShaderFactory& shaderFactory) override;

    virtual void SetupView(const donut::engine::IView* view) override;
    virtual void SetupSplitState(const TerrainMeshView* terrainView, nvrhi::ComputeState& state) override;
    virtual void SetupMergeState(const TerrainMeshView* terrainView, nvrhi::ComputeState& state) override;
    virtual void SetupPushConstants(nvrhi::ICommandList* commandList) override;

private:
    nvrhi::BindingSetHandle FindOrCreateBindingSet(const TerrainMeshView* key);

private:
    nvrhi::ShaderHandle m_SplitShader, m_MergeShader;
    nvrhi::ComputePipelineHandle m_SplitPipeline, m_MergePipeline;

    nvrhi::BindingLayoutHandle m_BindingLayout;

    std::unordered_map<const TerrainMeshView*, nvrhi::BindingSetHandle> m_BindingSets;
};
