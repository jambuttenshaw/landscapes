#pragma once

#include <donut/engine/TextureCache.h>
#include <donut/engine/SceneGraph.h>

#include "terrain/Terrain.h"


struct UIData;

class LandscapesScene
{
public:
    LandscapesScene(UIData& ui);
	bool Init(nvrhi::IDevice* device, nvrhi::ICommandList* commandList, donut::engine::TextureCache* textureCache);

    inline const std::shared_ptr<Terrain>& GetTerrain() const { return m_Terrain; }
    inline const std::shared_ptr<donut::engine::MeshInstance>& GetCubeMeshInstance() const { return m_CubeMeshInstance; }

    inline const std::shared_ptr<donut::engine::SceneGraph>& GetSceneGraph() const { return m_SceneGraph; }

    inline const std::vector<std::shared_ptr<donut::engine::Light>>& GetLights() const { return m_SceneGraph->GetLights(); }


    void Animate(float deltaTime);

private:

    nvrhi::BufferHandle CreateGeometryBuffer(
        nvrhi::IDevice* device, nvrhi::ICommandList* commandList, const char* debugName,
        const void* data, uint64_t dataSize, bool isVertexBuffer, bool isInstanceBuffer
    );

    nvrhi::BufferHandle CreateMaterialConstantBuffer(
        nvrhi::IDevice* device, nvrhi::ICommandList* commandList, const std::shared_ptr<donut::engine::Material> material
    );

private:
    UIData& m_UI;

	std::shared_ptr<donut::engine::SceneGraph> m_SceneGraph;

	std::shared_ptr<donut::engine::Material> m_GreyMaterial;
	std::shared_ptr<donut::engine::Material> m_GreenMaterial;

    std::shared_ptr<donut::engine::BufferGroup> m_CubeBuffers;
	std::shared_ptr<donut::engine::MeshInfo> m_CubeMeshInfo;
	std::shared_ptr<donut::engine::MeshInstance> m_CubeMeshInstance;

    std::shared_ptr<Terrain> m_Terrain;

    std::shared_ptr<donut::engine::DirectionalLight> m_SunLight;
};
