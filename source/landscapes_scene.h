#pragma once

#include <donut/engine/TextureCache.h>
#include <donut/engine/SceneGraph.h>


class LandscapesScene
{
public:
	bool Init(nvrhi::IDevice* device, nvrhi::ICommandList* commandList, donut::engine::TextureCache* textureCache);


    inline const std::shared_ptr<donut::engine::MeshInstance>& GetMeshInstance() const
    {
        return m_MeshInstance;
    }

    inline const std::shared_ptr<donut::engine::SceneGraph>& GetSceneGraph() const
    {
        return m_SceneGraph;
    }

    inline const std::vector<std::shared_ptr<donut::engine::Light>>& GetLights() const
    {
        return m_SceneGraph->GetLights();
    }

private:

    nvrhi::BufferHandle CreateGeometryBuffer(
        nvrhi::IDevice* device, nvrhi::ICommandList* commandList, const char* debugName,
        const void* data, uint64_t dataSize, bool isVertexBuffer, bool isInstanceBuffer
    );

    nvrhi::BufferHandle CreateMaterialConstantBuffer(
        nvrhi::IDevice* device, nvrhi::ICommandList* commandList, const std::shared_ptr<donut::engine::Material> material
    );

private:
	std::shared_ptr<donut::engine::SceneGraph> m_SceneGraph;

	std::shared_ptr<donut::engine::Material> m_Material;

    std::shared_ptr<donut::engine::BufferGroup> m_Buffers;

	std::shared_ptr<donut::engine::MeshInfo> m_MeshInfo;
	std::shared_ptr<donut::engine::MeshInstance> m_MeshInstance;
};
