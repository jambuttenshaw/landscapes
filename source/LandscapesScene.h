#pragma once

#include <donut/engine/TextureCache.h>
#include <donut/engine/SceneGraph.h>
#include <donut/engine/ShaderFactory.h>
#include <donut/engine/Scene.h>

#include "terrain/Terrain.h"


struct UIData;

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
        std::shared_ptr<donut::engine::SceneTypeFactory> sceneTypeFactory);

protected:

    virtual void CreateMeshBuffers(nvrhi::ICommandList* commandList) override;

    virtual bool LoadCustomData(Json::Value& rootNode, tf::Executor* executor) override;

private:
    UIData& m_UI;
};
