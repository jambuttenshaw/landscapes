#include "LandscapesScene.h"

#include <nvrhi/utils.h>
#include <json/value.h>
#include <donut/core/json.h>
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

                // LoadTextureFromFileDeferred always returns a shared pointer to TextureData
                const auto& textureData = std::static_pointer_cast<engine::TextureData>(terrainMesh->HeightmapTexture);
                float2 heightmapResolution {
					static_cast<float>(textureData->width),
                    static_cast<float>(textureData->height)
                };

                TerrainConstants terrainConstants;
                terrainConstants.TerrainExtentsAndInvExtents = float4(terrainMesh->HeightmapExtents, 1.0f / terrainMesh->HeightmapExtents);
                terrainConstants.HeightmapResolutionAndInvResolution = float4(heightmapResolution, 1.0f / heightmapResolution);
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

bool LandscapesScene::LoadCustomData(Json::Value& rootNode, const std::filesystem::path& fileName, tf::Executor* executor)
{
    auto sceneGraph = std::dynamic_pointer_cast<LandscapesSceneGraph>(GetSceneGraph());
    if (!sceneGraph)
    {
        log::fatal("Scene graph was not of type LandscapesSceneGraph: cannot process terrains.");
	    return false;
    }

    LoadTerrain(rootNode["terrains"], fileName, *sceneGraph);

    return true;
}

void LandscapesScene::LoadTerrain(const Json::Value& terrainList, const std::filesystem::path& fileName, LandscapesSceneGraph& sceneGraph)
{
	for (const auto& src : terrainList)
	{
		if (!src.isObject())
		{
			log::warning("Non-object found in the terrains list.");
            continue;
		}

        auto terrainMesh = std::make_shared<TerrainMeshInfo>();

        if (const auto& extents = src["extents"]; !extents.isNull())
	        extents >> terrainMesh->HeightmapExtents;

        if (const auto& height = src["height"]; !height.isNull())
            height >> terrainMesh->HeightmapHeightScale;

        if (const auto& path = src["path"]; !path.isNull())
        {
	        std::string pathStr;
            path >> pathStr;
            terrainMesh->HeightmapTexturePath = fileName / pathStr;
        }

        for (const auto& viewSrc : src["views"])
        {
	        if (!viewSrc.isObject())
	        {
                log::warning("Non-object found in the view list.");
                continue;
	        }

            auto& view = terrainMesh->TerrainViews.emplace_back();

            if (const auto& maxDepth = viewSrc["maxDepth"]; !maxDepth.isNull())
                maxDepth >> view.MaxDepth;

            if (const auto& initDepth = viewSrc["initDepth"]; !initDepth.isNull())
                initDepth >> view.InitDepth;

            const auto& tessellationScheme = viewSrc["tessellationScheme"];
            if (!tessellationScheme.isNull() && tessellationScheme.isString())
            {
	            if (tessellationScheme == "primary")
	            {
		            view.TessellationScheme = m_TerrainTessellationPass;
	            }
                else
                {
	                log::warning("Unknown tessellation scheme: '%s'", tessellationScheme.asCString());
                }
            }
        }

        sceneGraph.AddTerrainMesh(std::move(terrainMesh));
	}
}
