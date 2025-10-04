#include "LandscapesScene.h"

#include <nvrhi/utils.h>
#include <donut/app/ApplicationBase.h>

#include "UserInterface.h"
#include "engine/LandscapesSceneGraph.h"
#include "terrain/TerrainTessellation.h"

using namespace donut;
using namespace donut::math;

#include "TerrainShaders.h"


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
            if (!terrainMesh->buffers)
            {
                terrainMesh->buffers = std::make_shared<engine::BufferGroup>();
            }

            if (!terrainMesh->HeightmapTexture)
            {
                terrainMesh->HeightmapTexture = m_TextureCache->LoadTextureFromFileDeferred(terrainMesh->HeightmapTexturePath, true);
            }

            if (!terrainMesh->TerrainCB)
            {
                terrainMesh->TerrainCB = m_Device->createBuffer(nvrhi::utils::CreateStaticConstantBufferDesc(
                    sizeof(TerrainConstants), "TerrainConstants"
                ));

                TerrainConstants terrainConstants;
                terrainConstants.TerrainExtentsAndInvExtents = float4(terrainMesh->HeightmapExtents, 1.0f / terrainMesh->HeightmapExtents);
                terrainConstants.HeightmapResolutionAndInvResolution = float4(
			    		static_cast<float2>(terrainMesh->HeightmapResolution),
			    		1.0f / static_cast<float2>(terrainMesh->HeightmapResolution));
                terrainConstants.HeightScaleAndInvScale = float2(terrainMesh->HeightmapHeightScale, 1.0f / terrainMesh->HeightmapHeightScale);

                commandList->beginTrackingBufferState(terrainMesh->TerrainCB, nvrhi::ResourceStates::CopyDest);
                commandList->writeBuffer(terrainMesh->TerrainCB, &terrainConstants, sizeof(terrainConstants));
                commandList->setPermanentBufferState(terrainMesh->TerrainCB, nvrhi::ResourceStates::ConstantBuffer);
            }
	    }
    }

    for (auto& meshInstance : m_SceneGraph->GetMeshInstances())
    {
	    if (const auto& terrainMeshInstance = std::dynamic_pointer_cast<TerrainMeshInstance>(meshInstance))
	    {
		    terrainMeshInstance->CreateBuffers(m_Device, commandList);
	    }
    }
}

bool LandscapesScene::LoadCustomData(Json::Value& rootNode, tf::Executor* executor)
{
    auto terrainMesh = std::make_shared<TerrainMeshInfo>();
    terrainMesh->HeightmapResolution = { 1024, 1024 };
    terrainMesh->HeightmapExtents = { 262.28f, 262.28f };
    terrainMesh->HeightmapHeightScale = 155.23f;
    terrainMesh->HeightmapTexturePath = app::GetDirectoryWithExecutable().parent_path() / "media/test_heightmap.png";
    terrainMesh->TerrainViews.emplace_back(TerrainMeshViewDesc{ .MaxDepth = 20, .InitDepth = 10, .TessellationScheme = m_TerrainTessellationPass });
    if (auto graph = std::dynamic_pointer_cast<LandscapesSceneGraph>(GetSceneGraph()))
    {
        graph->AddTerrainMesh(terrainMesh);
    }

    //auto terrainNode = std::make_shared<engine::SceneGraphNode>();
    //m_SceneGraph->Attach(m_SceneGraph->GetRootNode(), terrainNode);
    //terrainNode->SetName("TerrainNode");
    //
    //auto terrainInstance = std::make_shared<TerrainMeshInstance>(terrainMesh);
    //terrainNode->SetLeaf(terrainInstance);
    //terrainInstance->SetName("TerrainMeshInstance");

    return true;
}
