#include "LandscapesScene.h"

#include <nvrhi/common/misc.h>

#include <donut/app/ApplicationBase.h>
#include <donut/shaders/bindless.h>

#include "UserInterface.h"
#include "terrain/TerrainTessellation.h"

using namespace donut;
using namespace donut::math;


struct LandscapesScene::Resources
{
    std::vector<InstanceData> instanceData;
};

LandscapesScene::LandscapesScene(UIData& ui, nvrhi::IDevice* device, donut::engine::ShaderFactory& shaderFactory, std::shared_ptr<donut::engine::CommonRenderPasses> commonPasses)
	: m_UI(ui)
	, m_Device(device)
{
    m_TerrainTessellationPass_PrimaryView = std::make_shared<PrimaryViewTerrainTessellationPass>(device, commonPasses);
    m_TerrainTessellationPass_PrimaryView->Init(shaderFactory);

    m_UI.TerrainSubdivisionLevel = static_cast<int>(m_TerrainTessellationPass_PrimaryView->GetSubdivisionLevel());
    m_UI.TerrainPrimitivePixelLength = m_TerrainTessellationPass_PrimaryView->GetPrimitivePixelLength();
}

bool LandscapesScene::Init(nvrhi::ICommandList* commandList, donut::engine::TextureCache* textureCache)
{
    m_Resources = std::make_shared<Resources>();

    m_SceneGraph = std::make_shared<engine::SceneGraph>();
    auto rootNode = std::make_shared<engine::SceneGraphNode>();
    m_SceneGraph->SetRootNode(rootNode);
    rootNode->SetName("Root");

    {
        TerrainMeshInfo::CreateParams createParams{};
        createParams.HeightmapResolution = { 1024, 1024 };
        createParams.HeightmapExtents = { 262.28f, 262.28f };
        createParams.HeightmapHeightScale = 155.23f;
        createParams.HeightmapTexturePath = app::GetDirectoryWithExecutable().parent_path() / "media/test_heightmap.png";
        createParams.Views.emplace_back(TerrainMeshViewDesc{ .MaxDepth = 20, .InitDepth = 10, .TessellationScheme = m_TerrainTessellationPass_PrimaryView });
        m_Terrain = std::make_shared<TerrainMeshInfo>(m_Device, commandList, textureCache, createParams);

        auto terrainNode = std::make_shared<engine::SceneGraphNode>();
        m_SceneGraph->Attach(rootNode, terrainNode);
        terrainNode->SetName("TerrainNode");

        m_TerrainInstance = std::make_shared<TerrainMeshInstance>(m_Terrain);
        m_TerrainInstance->Init(m_Device, commandList);
        terrainNode->SetLeaf(m_TerrainInstance);
        m_TerrainInstance->SetName("TerrainMeshInstance");
    }

    m_SunLight = std::make_shared<engine::DirectionalLight>();
    m_SceneGraph->AttachLeafNode(rootNode, m_SunLight);

    m_SunLight->SetDirection(static_cast<double3>(m_UI.LightDirection));
    m_SunLight->angularSize = 0.53f;
    m_SunLight->irradiance = 1.f;
    m_SunLight->SetName("Sun");

    return true;
}

void LandscapesScene::Animate(float deltaTime)
{
    m_SunLight->SetDirection(static_cast<double3>(m_UI.LightDirection));

    m_TerrainTessellationPass_PrimaryView->SetSubdivisionLevel(static_cast<uint32_t>(m_UI.TerrainSubdivisionLevel));
    m_TerrainTessellationPass_PrimaryView->SetPrimitivePixelLength(m_UI.TerrainPrimitivePixelLength);
}

void LandscapesScene::Refresh(nvrhi::ICommandList* commandList, uint frameIndex)
{
    // Refresh scene graph
	{
        m_SceneStructureChanged = m_SceneGraph->HasPendingStructureChanges();
        m_SceneTransformsChanged = m_SceneGraph->HasPendingTransformChanges();
		m_SceneGraph->Refresh(frameIndex);
	}

    // Update buffers
	{
        constexpr size_t allocationGranularity = 1024;
        bool arraysAllocated = false;

		if (m_SceneGraph->GetMeshInstances().size() > m_Resources->instanceData.size())
		{
            m_Resources->instanceData.resize(nvrhi::align<size_t>(m_SceneGraph->GetMeshInstances().size(), allocationGranularity));
            m_InstanceBuffer = CreateInstanceBuffer();
            arraysAllocated = true;
		}

        if (m_SceneStructureChanged || arraysAllocated)
        {
	        for (const auto& mesh : m_SceneGraph->GetMeshes())
	        {
                mesh->buffers->instanceBuffer = m_InstanceBuffer;
	        }
        }

        if (m_SceneStructureChanged || m_SceneTransformsChanged || arraysAllocated)
        {
	        for (const auto& instance : m_SceneGraph->GetMeshInstances())
	        {
                engine::SceneGraphNode* node = instance->GetNode();
                if (!node)
                    continue;

                InstanceData& instanceData = m_Resources->instanceData[instance->GetInstanceIndex()];
                affineToColumnMajor(node->GetLocalToWorldTransformFloat(), instanceData.transform);
                affineToColumnMajor(node->GetPrevLocalToWorldTransformFloat(), instanceData.prevTransform);

                const auto& mesh = instance->GetMesh();
                instanceData.firstGeometryInstanceIndex = instance->GetGeometryInstanceIndex();
                instanceData.firstGeometryIndex = mesh->geometries.empty() ? -1 : mesh->geometries[0]->globalGeometryIndex;
                instanceData.numGeometries = static_cast<uint>(mesh->geometries.size());
                instanceData.flags = 0u;

                if (mesh->type == engine::MeshType::CurveDisjointOrthogonalTriangleStrips)
                {
                    instanceData.flags |= InstanceFlags_CurveDisjointOrthogonalTriangleStrips;
                }
	        }

            commandList->writeBuffer(m_InstanceBuffer, m_Resources->instanceData.data(), m_Resources->instanceData.size() * sizeof(InstanceData));
        }
	}
}


nvrhi::BufferHandle LandscapesScene::CreateInstanceBuffer() const
{
    nvrhi::BufferDesc bufferDesc;
    bufferDesc.byteSize = sizeof(InstanceData) * m_Resources->instanceData.size();
    bufferDesc.debugName = "Instances";
    bufferDesc.structStride = sizeof(InstanceData);
    bufferDesc.canHaveRawViews = true;
    bufferDesc.canHaveUAVs = true;
    bufferDesc.isVertexBuffer = true;
    bufferDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    bufferDesc.keepInitialState = true;

    return m_Device->createBuffer(bufferDesc);
}
