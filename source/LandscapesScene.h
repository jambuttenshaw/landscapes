#pragma once

#include <donut/engine/TextureCache.h>
#include <donut/engine/SceneGraph.h>
#include <donut/engine/ShaderFactory.h>

#include "terrain/Terrain.h"


struct UIData;

class PrimaryViewTerrainTessellationPass;


class LandscapesScene
{
public:
    LandscapesScene(UIData& ui, nvrhi::IDevice* device, donut::engine::ShaderFactory& shaderFactory, std::shared_ptr<donut::engine::CommonRenderPasses> commonPasses);
	bool Init(nvrhi::ICommandList* commandList, donut::engine::TextureCache* textureCache);

    inline const std::shared_ptr<donut::engine::SceneGraph>& GetSceneGraph() const { return m_SceneGraph; }

    void Animate(float deltaTime);

    void Refresh(nvrhi::ICommandList* commandList, uint frameIndex);

protected:

    nvrhi::BufferHandle CreateInstanceBuffer() const;

private:
    UIData& m_UI;
    nvrhi::DeviceHandle m_Device;

	std::shared_ptr<donut::engine::SceneGraph> m_SceneGraph;
    nvrhi::BufferHandle m_InstanceBuffer;

    bool m_SceneStructureChanged = false;
    bool m_SceneTransformsChanged = false;

    struct Resources; // Hide the implementation to avoid including <material_cb.h> and <bindless.h> here
    std::shared_ptr<Resources> m_Resources;

    std::shared_ptr<donut::engine::DirectionalLight> m_SunLight;

    std::shared_ptr<TerrainMeshInfo> m_Terrain;
    std::shared_ptr<TerrainMeshInstance> m_TerrainInstance;

    std::shared_ptr<PrimaryViewTerrainTessellationPass> m_TerrainTessellationPass_PrimaryView;
};
