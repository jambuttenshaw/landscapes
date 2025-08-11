#pragma once

#include <donut/engine/ShaderFactory.h>
#include <donut/engine/View.h>

#include "donut/engine/CommonRenderPasses.h"


class TerrainMeshView;


class ITerrainTessellationPass
{
public:
    enum SubdivisionPassTypes : uint8_t
    {
	    Subdivision_Split = 0,
        Subdivision_Merge,
    };

public:
    ITerrainTessellationPass(nvrhi::DeviceHandle device, std::shared_ptr<donut::engine::CommonRenderPasses> commonPasses);
    virtual ~ITerrainTessellationPass() = default;

    virtual void Init(donut::engine::ShaderFactory& shaderFactory) = 0;

    virtual void SetupView(nvrhi::ICommandList* commandList, const TerrainMeshView* terrainView, const donut::engine::IView* view) = 0;
    virtual void SetupSubdivisionState(const TerrainMeshView* terrainView, SubdivisionPassTypes subdivisionPass, nvrhi::ComputeState& state) = 0;
    virtual void SetupPushConstants(nvrhi::ICommandList* commandList, const TerrainMeshView* terrainView) = 0;

protected:
    nvrhi::DeviceHandle m_Device;
    std::shared_ptr<donut::engine::CommonRenderPasses> m_CommonPasses;
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


struct UIData;

class PrimaryViewTerrainTessellationPass : public ITerrainTessellationPass
{
public:
    PrimaryViewTerrainTessellationPass(nvrhi::DeviceHandle device, std::shared_ptr<donut::engine::CommonRenderPasses> commonPasses, UIData& ui);

    void Init(donut::engine::ShaderFactory& shaderFactory) override;

    virtual void SetupView(nvrhi::ICommandList* commandList, const TerrainMeshView* terrainView, const donut::engine::IView* view) override;
    virtual void SetupSubdivisionState(const TerrainMeshView* terrainView, SubdivisionPassTypes subdivisionPass, nvrhi::ComputeState& state) override;
    virtual void SetupPushConstants(nvrhi::ICommandList* commandList, const TerrainMeshView* terrainView) override;

    [[nodiscard]] inline uint32_t GetSubdivisionLevel() const { return m_SubdivisionLevel; }
    [[nodiscard]] inline float GetPrimitivePixelLength() const { return m_PrimitivePixelLength; }

    inline void SetSubdivisionLevel(uint32_t subdivisionLevel) { m_SubdivisionLevel = subdivisionLevel; }
    inline void SetPrimitivePixelLength(float primitivePixelLength) { m_PrimitivePixelLength = primitivePixelLength; }

private:
    nvrhi::BindingSetHandle FindOrCreateBindingSet(const TerrainMeshView* key);

private:
	UIData& m_UI;

    nvrhi::ShaderHandle m_SplitShader, m_MergeShader;
    nvrhi::ComputePipelineHandle m_SplitPipeline, m_MergePipeline;

    nvrhi::BindingLayoutHandle m_ViewBindingLayout;
    nvrhi::BindingLayoutHandle m_TerrainBindingLayout;

    std::unordered_map<const TerrainMeshView*, nvrhi::BindingSetHandle> m_TerrainBindingSets;

    nvrhi::BindingSetHandle m_ViewBindingSet;
    nvrhi::BufferHandle m_ViewCB;

    uint32_t m_SubdivisionLevel = 2;
    float m_PrimitivePixelLength = 5.0f;
};
