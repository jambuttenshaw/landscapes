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
    std::shared_ptr<donut::engine::SceneTypeFactory> sceneTypeFactory)
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
}

//    TerrainMeshInfo::CreateParams createParams{};
//    createParams.HeightmapResolution = { 1024, 1024 };
//    createParams.HeightmapExtents = { 262.28f, 262.28f };
//    createParams.HeightmapHeightScale = 155.23f;
//    createParams.HeightmapTexturePath = app::GetDirectoryWithExecutable().parent_path() / "media/test_heightmap.png";
//    createParams.Views.emplace_back(TerrainMeshViewDesc{ .MaxDepth = 20, .InitDepth = 10, .TessellationScheme = m_TerrainTessellationPass_PrimaryView });
//    m_Terrain = std::make_shared<TerrainMeshInfo>(m_Device, commandList, textureCache, createParams);
//
//    auto terrainNode = std::make_shared<engine::SceneGraphNode>();
//    m_SceneGraph->Attach(rootNode, terrainNode);
//    terrainNode->SetName("TerrainNode");
//
//    m_TerrainInstance = std::make_shared<TerrainMeshInstance>(m_Terrain);
//    m_TerrainInstance->Init(m_Device, commandList);
//    terrainNode->SetLeaf(m_TerrainInstance);
//    m_TerrainInstance->SetName("TerrainMeshInstance");

void LandscapesScene::CreateMeshBuffers(nvrhi::ICommandList* commandList)
{
	
}
