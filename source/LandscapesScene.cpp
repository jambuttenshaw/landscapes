#include "LandscapesScene.h"

#include "UserInterface.h"
#include "donut/app/ApplicationBase.h"

using namespace donut;
using namespace donut::math;

#include <donut/shaders/material_cb.h>
#include <donut/shaders/bindless.h>


static const float3 g_Positions[] = {
    {-0.5f,  0.5f, -0.5f}, // front face
    { 0.5f, -0.5f, -0.5f},
    {-0.5f, -0.5f, -0.5f},
    { 0.5f,  0.5f, -0.5f},
    
    { 0.5f, -0.5f, -0.5f}, // right side face
    { 0.5f,  0.5f,  0.5f},
    { 0.5f, -0.5f,  0.5f},
    { 0.5f,  0.5f, -0.5f},

    {-0.5f,  0.5f,  0.5f}, // left side face
    {-0.5f, -0.5f, -0.5f},
    {-0.5f, -0.5f,  0.5f},
    {-0.5f,  0.5f, -0.5f},

    { 0.5f,  0.5f,  0.5f}, // back face
    {-0.5f, -0.5f,  0.5f},
    { 0.5f, -0.5f,  0.5f},
    {-0.5f,  0.5f,  0.5f},

    {-0.5f,  0.5f, -0.5f}, // top face
    { 0.5f,  0.5f,  0.5f},
    { 0.5f,  0.5f, -0.5f},
    {-0.5f,  0.5f,  0.5f},

    { 0.5f, -0.5f,  0.5f}, // bottom face
    {-0.5f, -0.5f, -0.5f},
    { 0.5f, -0.5f, -0.5f},
    {-0.5f, -0.5f,  0.5f},
};

static const float2 g_TexCoords[] = {
    {0.0f, 0.0f}, // front face
    {1.0f, 1.0f},
    {0.0f, 1.0f},
    {1.0f, 0.0f},
    
    {0.0f, 1.0f}, // right side face
    {1.0f, 0.0f},
    {1.0f, 1.0f},
    {0.0f, 0.0f},

    {0.0f, 0.0f}, // left side face
    {1.0f, 1.0f},
    {0.0f, 1.0f},
    {1.0f, 0.0f},

    {0.0f, 0.0f}, // back face
    {1.0f, 1.0f},
    {0.0f, 1.0f},
    {1.0f, 0.0f},

    {0.0f, 1.0f}, // top face
    {1.0f, 0.0f},
    {1.0f, 1.0f},
    {0.0f, 0.0f},

    {1.0f, 1.0f}, // bottom face
    {0.0f, 0.0f},
    {1.0f, 0.0f},
    {0.0f, 1.0f}, 
};

static const uint g_Normals[] = {
    vectorToSnorm8(float4(0.0f, 0.0f, -1.0f, 0.0f)), // front face
    vectorToSnorm8(float4(0.0f, 0.0f, -1.0f, 0.0f)),
    vectorToSnorm8(float4(0.0f, 0.0f, -1.0f, 0.0f)),
    vectorToSnorm8(float4(0.0f, 0.0f, -1.0f, 0.0f)),
    
    vectorToSnorm8(float4(1.0f, 0.0f, 0.0f, 0.0f)), // right side face
    vectorToSnorm8(float4(1.0f, 0.0f, 0.0f, 0.0f)),
    vectorToSnorm8(float4(1.0f, 0.0f, 0.0f, 0.0f)),
    vectorToSnorm8(float4(1.0f, 0.0f, 0.0f, 0.0f)),

    vectorToSnorm8(float4(-1.0f, 0.0f, 0.0f, 0.0f)), // left side face
    vectorToSnorm8(float4(-1.0f, 0.0f, 0.0f, 0.0f)),
    vectorToSnorm8(float4(-1.0f, 0.0f, 0.0f, 0.0f)),
    vectorToSnorm8(float4(-1.0f, 0.0f, 0.0f, 0.0f)),

    vectorToSnorm8(float4(0.0f, 0.0f, 1.0f, 0.0f)), // back face
    vectorToSnorm8(float4(0.0f, 0.0f, 1.0f, 0.0f)),
    vectorToSnorm8(float4(0.0f, 0.0f, 1.0f, 0.0f)),
    vectorToSnorm8(float4(0.0f, 0.0f, 1.0f, 0.0f)),

    vectorToSnorm8(float4(0.0f, 1.0f, 0.0f, 0.0f)), // top face
    vectorToSnorm8(float4(0.0f, 1.0f, 0.0f, 0.0f)),
    vectorToSnorm8(float4(0.0f, 1.0f, 0.0f, 0.0f)),
    vectorToSnorm8(float4(0.0f, 1.0f, 0.0f, 0.0f)),

    vectorToSnorm8(float4(0.0f, -1.0f, 0.0f, 0.0f)), // bottom face
    vectorToSnorm8(float4(0.0f, -1.0f, 0.0f, 0.0f)),
    vectorToSnorm8(float4(0.0f, -1.0f, 0.0f, 0.0f)),
    vectorToSnorm8(float4(0.0f, -1.0f, 0.0f, 0.0f)), 
};

static const uint g_Tangents[] = {
    vectorToSnorm8(float4(1.0f, 0.0f, 0.0f, 1.0f)), // front face
    vectorToSnorm8(float4(1.0f, 0.0f, 0.0f, 1.0f)),
    vectorToSnorm8(float4(1.0f, 0.0f, 0.0f, 1.0f)),
    vectorToSnorm8(float4(1.0f, 0.0f, 0.0f, 1.0f)),
    
    vectorToSnorm8(float4(0.0f, 0.0f, 1.0f, 1.0f)), // right side face
    vectorToSnorm8(float4(0.0f, 0.0f, 1.0f, 1.0f)),
    vectorToSnorm8(float4(0.0f, 0.0f, 1.0f, 1.0f)),
    vectorToSnorm8(float4(0.0f, 0.0f, 1.0f, 1.0f)),

    vectorToSnorm8(float4(0.0f, 0.0f, -1.0f, 1.0f)), // left side face
    vectorToSnorm8(float4(0.0f, 0.0f, -1.0f, 1.0f)),
    vectorToSnorm8(float4(0.0f, 0.0f, -1.0f, 1.0f)),
    vectorToSnorm8(float4(0.0f, 0.0f, -1.0f, 1.0f)),

    vectorToSnorm8(float4(-1.0f, 0.0f, 0.0f, 1.0f)), // back face
    vectorToSnorm8(float4(-1.0f, 0.0f, 0.0f, 1.0f)),
    vectorToSnorm8(float4(-1.0f, 0.0f, 0.0f, 1.0f)),
    vectorToSnorm8(float4(-1.0f, 0.0f, 0.0f, 1.0f)),

    vectorToSnorm8(float4(1.0f, 0.0f, 0.0f, 1.0f)), // top face
    vectorToSnorm8(float4(1.0f, 0.0f, 0.0f, 1.0f)),
    vectorToSnorm8(float4(1.0f, 0.0f, 0.0f, 1.0f)),
    vectorToSnorm8(float4(1.0f, 0.0f, 0.0f, 1.0f)),

    vectorToSnorm8(float4(1.0f, 0.0f, 0.0f, 1.0f)), // bottom face
    vectorToSnorm8(float4(1.0f, 0.0f, 0.0f, 1.0f)),
    vectorToSnorm8(float4(1.0f, 0.0f, 0.0f, 1.0f)),
    vectorToSnorm8(float4(1.0f, 0.0f, 0.0f, 1.0f)), 
};

static const uint32_t g_Indices[] = {
     0,  1,  2,   0,  3,  1, // front face
     4,  5,  6,   4,  7,  5, // left face
     8,  9, 10,   8, 11,  9, // right face
    12, 13, 14,  12, 15, 13, // back face
    16, 17, 18,  16, 19, 17, // top face
    20, 21, 22,  20, 23, 21, // bottom face
};


LandscapesScene::LandscapesScene(UIData& ui)
	: m_UI(ui)
{
}

bool LandscapesScene::Init(nvrhi::IDevice* device, nvrhi::ICommandList* commandList, donut::engine::TextureCache* textureCache)
{
    m_SceneGraph = std::make_shared<engine::SceneGraph>();
    auto rootNode = std::make_shared<engine::SceneGraphNode>();
    m_SceneGraph->SetRootNode(rootNode);

    {
        m_GreyMaterial = std::make_shared<engine::Material>();
        m_GreyMaterial->name = "GreyMaterial";
        m_GreyMaterial->useSpecularGlossModel = true;
        m_GreyMaterial->baseOrDiffuseColor = float3(0.7f, 0.7f, 0.8f);
        m_GreyMaterial->materialConstants = CreateMaterialConstantBuffer(device, commandList, m_GreyMaterial);

        m_GreenMaterial = std::make_shared<engine::Material>();
        m_GreenMaterial->name = "GreenMaterial";
        m_GreenMaterial->useSpecularGlossModel = true;
        m_GreenMaterial->baseOrDiffuseColor = float3(0.6f, 0.9f, 0.3f);
        m_GreenMaterial->materialConstants = CreateMaterialConstantBuffer(device, commandList, m_GreenMaterial);
    }

    {
	    m_CubeBuffers = std::make_shared<engine::BufferGroup>();
	    m_CubeBuffers->indexBuffer = CreateGeometryBuffer(device, commandList, "IndexBuffer", g_Indices, sizeof(g_Indices), false, false);

	    uint64_t vertexBufferSize = 0;
	    m_CubeBuffers->getVertexBufferRange(engine::VertexAttribute::Position).setByteOffset(vertexBufferSize).setByteSize(sizeof(g_Positions)); vertexBufferSize += sizeof(g_Positions);
	    m_CubeBuffers->getVertexBufferRange(engine::VertexAttribute::TexCoord1).setByteOffset(vertexBufferSize).setByteSize(sizeof(g_TexCoords)); vertexBufferSize += sizeof(g_TexCoords);
	    m_CubeBuffers->getVertexBufferRange(engine::VertexAttribute::Normal).setByteOffset(vertexBufferSize).setByteSize(sizeof(g_Normals)); vertexBufferSize += sizeof(g_Normals);
	    m_CubeBuffers->getVertexBufferRange(engine::VertexAttribute::Tangent).setByteOffset(vertexBufferSize).setByteSize(sizeof(g_Tangents)); vertexBufferSize += sizeof(g_Tangents);
	    m_CubeBuffers->vertexBuffer = CreateGeometryBuffer(device, commandList, "VertexBuffer", nullptr, vertexBufferSize, true, false);

	    commandList->beginTrackingBufferState(m_CubeBuffers->vertexBuffer, nvrhi::ResourceStates::CopyDest);
	    commandList->writeBuffer(m_CubeBuffers->vertexBuffer, g_Positions, sizeof(g_Positions), m_CubeBuffers->getVertexBufferRange(engine::VertexAttribute::Position).byteOffset);
	    commandList->writeBuffer(m_CubeBuffers->vertexBuffer, g_TexCoords, sizeof(g_TexCoords), m_CubeBuffers->getVertexBufferRange(engine::VertexAttribute::TexCoord1).byteOffset);
	    commandList->writeBuffer(m_CubeBuffers->vertexBuffer, g_Normals, sizeof(g_Normals), m_CubeBuffers->getVertexBufferRange(engine::VertexAttribute::Normal).byteOffset);
	    commandList->writeBuffer(m_CubeBuffers->vertexBuffer, g_Tangents, sizeof(g_Tangents), m_CubeBuffers->getVertexBufferRange(engine::VertexAttribute::Tangent).byteOffset);
	    commandList->setPermanentBufferState(m_CubeBuffers->vertexBuffer, nvrhi::ResourceStates::ShaderResource);

	    InstanceData instance{};
	    instance.transform = math::float3x4(transpose(math::affineToHomogeneous(math::translation(float3(0, 0.5f, 0.0f)))));
	    instance.prevTransform = instance.transform;
	    m_CubeBuffers->instanceBuffer = CreateGeometryBuffer(device, commandList, "VertexBufferTransform", &instance, sizeof(instance), false, true);

	    auto geometry = std::make_shared<engine::MeshGeometry>();
	    geometry->material = m_GreyMaterial;
	    geometry->numIndices = dim(g_Indices);
	    geometry->numVertices = dim(g_Positions);

	    m_CubeMeshInfo = std::make_shared<engine::MeshInfo>();
	    m_CubeMeshInfo->name = "CubeMesh";
	    m_CubeMeshInfo->buffers = m_CubeBuffers;
	    m_CubeMeshInfo->objectSpaceBounds = math::box3(math::float3(-0.5f), math::float3(0.5f));
	    m_CubeMeshInfo->totalIndices = geometry->numIndices;
	    m_CubeMeshInfo->totalVertices = geometry->numVertices;
	    m_CubeMeshInfo->geometries.push_back(geometry);
    }

    {
        Terrain::CreateParams createParams{};
        createParams.HeightmapResolution = { 1024, 1024 };
        createParams.HeightmapExtents = { 262.28f, 262.28f };
        createParams.HeightmapHeightScale = 155.23f;
        createParams.TerrainResolution = { 32, 32 };
        createParams.HeightmapTexturePath = app::GetDirectoryWithExecutable().parent_path() / "media/test_heightmap.png";

        m_Terrain = std::make_shared<Terrain>();
        if (!m_Terrain->Init(device, commandList, textureCache, m_SceneGraph.get(), createParams))
        {
	        return false;
        }

        m_UI.TerrainHeight = m_Terrain->GetHeightScale();
    }

    auto cubeNode = std::make_shared<engine::SceneGraphNode>();
    m_SceneGraph->Attach(rootNode, cubeNode);
    m_CubeMeshInstance = std::make_shared<engine::MeshInstance>(m_CubeMeshInfo);
    cubeNode->SetLeaf(m_CubeMeshInstance);
    cubeNode->SetName("CubeNode");

    m_SunLight = std::make_shared<engine::DirectionalLight>();
    m_SceneGraph->AttachLeafNode(rootNode, m_SunLight);

    m_SunLight->SetDirection(static_cast<double3>(m_UI.LightDirection));
    m_SunLight->angularSize = 0.53f;
    m_SunLight->irradiance = 1.f;
    m_SunLight->SetName("Sun");

    m_SceneGraph->Refresh(0);

    PrintSceneGraph(m_SceneGraph->GetRootNode());

    return true;
}

nvrhi::BufferHandle LandscapesScene::CreateGeometryBuffer(nvrhi::IDevice* device, nvrhi::ICommandList* commandList, const char* debugName, const void* data, uint64_t dataSize, bool isVertexBuffer, bool isInstanceBuffer)
{
    nvrhi::BufferHandle bufHandle;

    // The G-buffer fill pass accesses instance buffers as structured on DX12 and Vulkan, and as raw on DX11.
    bool const needStructuredBuffer = isInstanceBuffer && device->getGraphicsAPI() != nvrhi::GraphicsAPI::D3D11;

    nvrhi::BufferDesc desc;
    desc.byteSize = dataSize;
    desc.isIndexBuffer = !isVertexBuffer && !isInstanceBuffer;
    desc.canHaveRawViews = isVertexBuffer || isInstanceBuffer;
    desc.structStride = needStructuredBuffer ? sizeof(InstanceData) : 0;
    desc.debugName = debugName;
    desc.initialState = nvrhi::ResourceStates::CopyDest;
    bufHandle = device->createBuffer(desc);

    if (data)
    {
        commandList->beginTrackingBufferState(bufHandle, nvrhi::ResourceStates::CopyDest);
        commandList->writeBuffer(bufHandle, data, dataSize);
        commandList->setPermanentBufferState(bufHandle, (isVertexBuffer || isInstanceBuffer)
            ? nvrhi::ResourceStates::ShaderResource
            : nvrhi::ResourceStates::IndexBuffer);
    }

    return bufHandle;
}

nvrhi::BufferHandle LandscapesScene::CreateMaterialConstantBuffer(nvrhi::IDevice* device, nvrhi::ICommandList* commandList, const std::shared_ptr<donut::engine::Material> material)
{
    nvrhi::BufferDesc bufferDesc;
    bufferDesc.byteSize = sizeof(MaterialConstants);
    bufferDesc.debugName = material->name;
    bufferDesc.isConstantBuffer = true;
    bufferDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
    bufferDesc.keepInitialState = true;
    nvrhi::BufferHandle buffer = device->createBuffer(bufferDesc);

    MaterialConstants constants;
    material->FillConstantBuffer(constants);
    commandList->writeBuffer(buffer, &constants, sizeof(constants));

    return buffer;
}


void LandscapesScene::Animate(float deltaTime)
{
    m_SunLight->SetDirection(static_cast<double3>(m_UI.LightDirection));

    m_Terrain->SetHeightScale(m_UI.TerrainHeight);
}
