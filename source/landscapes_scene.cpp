#include "landscapes_scene.h"

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


bool LandscapesScene::Init(nvrhi::IDevice* device, nvrhi::ICommandList* commandList, donut::engine::TextureCache* textureCache)
{
    commandList->open();

    m_Buffers = std::make_shared<engine::BufferGroup>();
    m_Buffers->indexBuffer = CreateGeometryBuffer(device, commandList, "IndexBuffer", g_Indices, sizeof(g_Indices), false, false);

    uint64_t vertexBufferSize = 0;
    m_Buffers->getVertexBufferRange(engine::VertexAttribute::Position).setByteOffset(vertexBufferSize).setByteSize(sizeof(g_Positions)); vertexBufferSize += sizeof(g_Positions);
    m_Buffers->getVertexBufferRange(engine::VertexAttribute::TexCoord1).setByteOffset(vertexBufferSize).setByteSize(sizeof(g_TexCoords)); vertexBufferSize += sizeof(g_TexCoords);
    m_Buffers->getVertexBufferRange(engine::VertexAttribute::Normal).setByteOffset(vertexBufferSize).setByteSize(sizeof(g_Normals)); vertexBufferSize += sizeof(g_Normals);
    m_Buffers->getVertexBufferRange(engine::VertexAttribute::Tangent).setByteOffset(vertexBufferSize).setByteSize(sizeof(g_Tangents)); vertexBufferSize += sizeof(g_Tangents);
    m_Buffers->vertexBuffer = CreateGeometryBuffer(device, commandList, "VertexBuffer", nullptr, vertexBufferSize, true, false);

    commandList->beginTrackingBufferState(m_Buffers->vertexBuffer, nvrhi::ResourceStates::CopyDest);
    commandList->writeBuffer(m_Buffers->vertexBuffer, g_Positions, sizeof(g_Positions), m_Buffers->getVertexBufferRange(engine::VertexAttribute::Position).byteOffset);
    commandList->writeBuffer(m_Buffers->vertexBuffer, g_TexCoords, sizeof(g_TexCoords), m_Buffers->getVertexBufferRange(engine::VertexAttribute::TexCoord1).byteOffset);
    commandList->writeBuffer(m_Buffers->vertexBuffer, g_Normals, sizeof(g_Normals), m_Buffers->getVertexBufferRange(engine::VertexAttribute::Normal).byteOffset);
    commandList->writeBuffer(m_Buffers->vertexBuffer, g_Tangents, sizeof(g_Tangents), m_Buffers->getVertexBufferRange(engine::VertexAttribute::Tangent).byteOffset);
    commandList->setPermanentBufferState(m_Buffers->vertexBuffer, nvrhi::ResourceStates::ShaderResource);

    InstanceData instance{};
    instance.transform = math::float3x4(transpose(math::affineToHomogeneous(math::affine3::identity())));
    instance.prevTransform = instance.transform;
    m_Buffers->instanceBuffer = CreateGeometryBuffer(device, commandList, "VertexBufferTransform", &instance, sizeof(instance), false, true);

	m_Material = std::make_shared<engine::Material>();
	m_Material->name = "Plane Material";
    m_Material->useSpecularGlossModel = true;
    m_Material->materialConstants = CreateMaterialConstantBuffer(device, commandList, m_Material);

    commandList->close();
    device->executeCommandList(commandList);

    auto geometry = std::make_shared<engine::MeshGeometry>();
    geometry->material = m_Material;
    geometry->numIndices = dim(g_Indices);
    geometry->numVertices = dim(g_Positions);

    m_MeshInfo = std::make_shared<engine::MeshInfo>();
    m_MeshInfo->name = "CubeMesh";
    m_MeshInfo->buffers = m_Buffers;
    m_MeshInfo->objectSpaceBounds = math::box3(math::float3(-0.5f), math::float3(0.5f));
    m_MeshInfo->totalIndices = geometry->numIndices;
    m_MeshInfo->totalVertices = geometry->numVertices;
    m_MeshInfo->geometries.push_back(geometry);

    m_SceneGraph = std::make_shared<engine::SceneGraph>();
    auto node = std::make_shared<engine::SceneGraphNode>();
    m_SceneGraph->SetRootNode(node);

    m_MeshInstance = std::make_shared<engine::MeshInstance>(m_MeshInfo);
    node->SetLeaf(m_MeshInstance);
    node->SetName("CubeNode");

    auto sunLight = std::make_shared<engine::DirectionalLight>();
    m_SceneGraph->AttachLeafNode(node, sunLight);

    sunLight->SetDirection(double3(0.1, -1.0, 0.2));
    sunLight->angularSize = 0.53f;
    sunLight->irradiance = 1.f;
    sunLight->SetName("Sun");

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
