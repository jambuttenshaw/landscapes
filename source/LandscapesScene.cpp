#include "LandscapesScene.h"

#include <donut/app/ApplicationBase.h>

#include "UserInterface.h"
#include "terrain/TerrainTessellation.h"

using namespace donut;
using namespace donut::math;


LandscapesScene::LandscapesScene(
    UIData& ui,
    nvrhi::IDevice* device,
    donut::engine::ShaderFactory& shaderFactory,
    std::shared_ptr<donut::vfs::IFileSystem> fs,
    std::shared_ptr<donut::engine::TextureCache> textureCache,
    std::shared_ptr<donut::engine::DescriptorTableManager> descriptorTable,
    std::shared_ptr<donut::engine::SceneTypeFactory> sceneTypeFactory,
    std::shared_ptr<donut::engine::CommonRenderPasses> commonPasses)
	: Scene(
		device,
		shaderFactory,
        std::move(fs),
        std::move(textureCache),
        std::move(descriptorTable),
        std::move(sceneTypeFactory)
    )
	, m_UI(ui)
{
    m_TerrainTessellationPass = std::make_shared<PrimaryViewTerrainTessellationPass>(device, std::move(commonPasses));
    m_TerrainTessellationPass->Init(shaderFactory);
}

void LandscapesScene::CreateMeshBuffers(nvrhi::ICommandList* commandList)
{
    Scene::CreateMeshBuffers(commandList);

    // Create and populate buffers for terrain mesh data (initialize CBTs)
    for (const auto& mesh : m_SceneGraph->GetMeshes())
    {
	    if (const auto& terrainMesh = std::dynamic_pointer_cast<TerrainMeshInfo>(mesh))
	    {
		    terrainMesh->Init(m_Device, commandList);
	    }
    }

    for (auto& meshInstance : m_SceneGraph->GetMeshInstances())
    {
	    if (const auto& terrainMeshInstance = std::dynamic_pointer_cast<TerrainMeshInstance>(meshInstance))
	    {
		    terrainMeshInstance->Init(m_Device, commandList);
	    }
    }
}

bool LandscapesScene::LoadCustomData(Json::Value& rootNode, tf::Executor* executor)
{
    TerrainMeshInfo::CreateParams createParams{};
    createParams.HeightmapResolution = { 1024, 1024 };
    createParams.HeightmapExtents = { 262.28f, 262.28f };
    createParams.HeightmapHeightScale = 155.23f;
    createParams.HeightmapTexturePath = app::GetDirectoryWithExecutable().parent_path() / "media/test_heightmap.png";
    createParams.Views.emplace_back(TerrainMeshViewDesc{ .MaxDepth = 20, .InitDepth = 10, .TessellationScheme = m_TerrainTessellationPass });
    auto terrainMesh = std::make_shared<TerrainMeshInfo>(*m_TextureCache, createParams);

    auto terrainNode = std::make_shared<engine::SceneGraphNode>();
    m_SceneGraph->Attach(m_SceneGraph->GetRootNode(), terrainNode);
    terrainNode->SetName("TerrainNode");

    auto terrainInstance = std::make_shared<TerrainMeshInstance>(terrainMesh);
    terrainNode->SetLeaf(terrainInstance);
    terrainInstance->SetName("TerrainMeshInstance");

    return true;
}
