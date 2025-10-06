#pragma once

#include <donut/engine/TextureCache.h>
#include <donut/engine/SceneGraph.h>
#include <donut/engine/ShaderFactory.h>
#include <donut/engine/Scene.h>

#include "terrain/Terrain.h"


struct UIData;

class LandscapesSceneGraph;
class PrimaryViewTerrainTessellationPass;


class LandscapesScene : public donut::engine::Scene
{
public:
    LandscapesScene(
		UIData& ui,
		nvrhi::IDevice* device,
        donut::engine::ShaderFactory& shaderFactory,
        std::shared_ptr<donut::vfs::IFileSystem> fs,
        std::shared_ptr<donut::engine::TextureCache> textureCache,
        std::shared_ptr<donut::engine::DescriptorTableManager> descriptorTable,
        std::shared_ptr<donut::engine::SceneTypeFactory> sceneTypeFactory,
        std::shared_ptr<donut::engine::CommonRenderPasses> commonPasses);

protected:

    virtual void CreateMeshBuffers(nvrhi::ICommandList* commandList) override;

    virtual bool LoadCustomData(Json::Value& rootNode, const std::filesystem::path& fileName, tf::Executor* executor) override;

private:
    void LoadTerrain(const Json::Value& terrainList, const std::filesystem::path& fileName, LandscapesSceneGraph& sceneGraph);

private:
    UIData& m_UI;

    std::shared_ptr<PrimaryViewTerrainTessellationPass> m_TerrainTessellationPass;
};
