#pragma once

#include <donut/engine/ShaderFactory.h>
#include <donut/engine/View.h>

class Terrain;

class TerrainTessellationPass
{
public:
    TerrainTessellationPass(nvrhi::IDevice* device);

    void Init(donut::engine::ShaderFactory& shaderFactory);

    // Run tessellation pipeline for a specific terrain
    // The tessellation scheme will be decided from the view's type
    void ExecuteForAllViews(
        nvrhi::ICommandList* commandList,
        const donut::engine::IView* view,
        const Terrain& terrain
    );

protected:
    nvrhi::DeviceHandle m_Device;

    // Shader handles
    enum Shaders : uint8_t
    {
        CBTDispatch = 0,
        LEBDispatch,
        Split,
        Merge,
        SumReductionPrePass,
        SumReduction,

        Shaders_Count
    };
    std::array<nvrhi::ShaderHandle, Shaders_Count> m_Shaders{};

    enum Pipelines : uint8_t
    {
        Pipelines_Count
    };
    std::array<nvrhi::ComputePipelineHandle, Pipelines_Count> m_Pipelines{};
};
