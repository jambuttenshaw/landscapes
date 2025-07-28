#pragma once

#include <donut/engine/ShaderFactory.h>
#include <donut/engine/View.h>


class TerrainMeshView;


class ITerrainTessellationPass
{
public:
    virtual ~ITerrainTessellationPass() = default;

    // TODO: These should just set up compute state, not actually submit commands
    // TODO: This lets the terrain tessellator remain in control of HOW work is submitted,
    // TODO: but the pass will determine WHAT work is performed
    virtual void ExecuteSplitPipeline() const = 0;
    virtual void ExecuteMergePipeline() const = 0;
};

class TerrainTessellator
{
public:
    TerrainTessellator(nvrhi::DeviceHandle device);

    void Init(donut::engine::ShaderFactory& shaderFactory);

    void ExecutePassForTerrainView(
        nvrhi::ICommandList* commandList,
        const donut::engine::IView* view,
        const ITerrainTessellationPass& pass,
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

    std::unordered_map<const TerrainMeshView*, std::array<nvrhi::BindingSetHandle, Bindings_Count>> m_BindingSets;
};


class PrimaryViewTerrainTessellationPass : public ITerrainTessellationPass
{
public:
    virtual void ExecuteSplitPipeline() const override {}
    void ExecuteMergePipeline() const override {}
};
