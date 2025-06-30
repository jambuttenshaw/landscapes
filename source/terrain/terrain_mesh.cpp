#include "terrain_mesh.h"

using namespace donut;
using namespace donut::math;

#include "donut/shaders/bindless.h"


void CreateTerrainMesh(
	uint2 resolution,
	std::vector<float3>& outPositions,
	std::vector<uint32_t>& outIndices
)
{
	const float2 fRes = static_cast<float2>(resolution);

	outPositions.clear();
	outPositions.resize(static_cast<size_t>(resolution.x * resolution.y));

	outIndices.clear();
	outIndices.resize(static_cast<size_t>(6 * (resolution.x - 1) * (resolution.y - 1)));

	uint vertex = 0;
	for (uint x = 0; x < resolution.x; x++)
	{
		for (uint y = 0; y < resolution.y; y++)
		{
			outPositions[vertex++] = {
				(static_cast<float>(x) / (fRes.x - 1.0f)) - 0.5f,
				0.0f,
				(static_cast<float>(y) / (fRes.y - 1.0f)) - 0.5f,
			};
		}
	}

	uint index = 0;
	for (uint x = 0; x < resolution.x - 1; x++)
	{
		for (uint y = 0; y < resolution.y - 1; y++)
		{
			outIndices[index + 0] = (y + 0) + resolution.x * (x + 0);
			outIndices[index + 1] = (y + 1) + resolution.x * (x + 1);
			outIndices[index + 2] = (y + 0) + resolution.x * (x + 1);

			outIndices[index + 3] = (y + 0) + resolution.x * (x + 0);
			outIndices[index + 4] = (y + 1) + resolution.x * (x + 0);
			outIndices[index + 5] = (y + 1) + resolution.x * (x + 1);

			index += 6;
		}
	}
}


TerrainMesh::TerrainMesh(uint2 resolution)
	: m_Resolution(resolution)
{
	assert(all(m_Resolution > uint2{0, 0}));
}

void TerrainMesh::InitResources(nvrhi::IDevice* device, nvrhi::ICommandList* commandList, uint32_t instanceCount)
{
	assert(instanceCount > 0);

	std::vector<float3> positions;
	std::vector<uint32_t> indices;

	CreateTerrainMesh(m_Resolution, positions, indices);

	m_Buffers = std::make_shared<engine::BufferGroup>();

	// Create vertex buffer
	{
		const uint64_t positionsByteSize = positions.size() * sizeof(positions[0]);

		m_Buffers->getVertexBufferRange(engine::VertexAttribute::Position).setByteOffset(0).setByteSize(positionsByteSize);

		nvrhi::BufferDesc vertexBufferDesc;
		vertexBufferDesc.byteSize = positionsByteSize;
		vertexBufferDesc.canHaveRawViews = true; // vertex buffers accessed via structured buffers
		vertexBufferDesc.structStride = sizeof(positions[0]);
		vertexBufferDesc.debugName = "TerrainVertexBuffer";
		vertexBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
		m_Buffers->vertexBuffer = device->createBuffer(vertexBufferDesc);

		commandList->beginTrackingBufferState(m_Buffers->vertexBuffer, nvrhi::ResourceStates::CopyDest);
		commandList->writeBuffer(m_Buffers->vertexBuffer, positions.data(), positionsByteSize);
		commandList->setPermanentBufferState(m_Buffers->vertexBuffer, nvrhi::ResourceStates::ShaderResource);
	}

	// Create index buffer
	{
		const uint64_t indicesByteSize = indices.size() * sizeof(indices[0]);

		nvrhi::BufferDesc indexBufferDesc;
		indexBufferDesc.byteSize = indicesByteSize;
		indexBufferDesc.isIndexBuffer = true;
		indexBufferDesc.debugName = "TerrainIndexBuffer";
		indexBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
		m_Buffers->indexBuffer = device->createBuffer(indexBufferDesc);

		commandList->beginTrackingBufferState(m_Buffers->indexBuffer, nvrhi::ResourceStates::CopyDest);
		commandList->writeBuffer(m_Buffers->indexBuffer, indices.data(), indicesByteSize);
		commandList->setPermanentBufferState(m_Buffers->indexBuffer, nvrhi::ResourceStates::IndexBuffer);
	}

	// Create instance buffer
	{
		std::vector<InstanceData> instances(instanceCount);
		for (auto& instanceData : instances)
		{
			instanceData.transform = float3x4(transpose(affineToHomogeneous(affine3::identity())));
			instanceData.prevTransform = instanceData.transform;
		}

		uint64_t instancesByteSize = instances.size() * sizeof(instances[0]);

		nvrhi::BufferDesc instanceBufferDesc;
		instanceBufferDesc.byteSize = instancesByteSize;
		instanceBufferDesc.canHaveRawViews = true;
		instanceBufferDesc.structStride = sizeof(instances[0]);
		instanceBufferDesc.debugName = "TerrainIndexBuffer";
		instanceBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
		m_Buffers->instanceBuffer = device->createBuffer(instanceBufferDesc);

		commandList->beginTrackingBufferState(m_Buffers->instanceBuffer, nvrhi::ResourceStates::CopyDest);
		commandList->writeBuffer(m_Buffers->instanceBuffer, instances.data(), instancesByteSize);
		commandList->setPermanentBufferState(m_Buffers->instanceBuffer, nvrhi::ResourceStates::ShaderResource);
	}

	// Create mesh info
	{
		auto geometry = std::make_shared<engine::MeshGeometry>();
		geometry->material = nullptr;
		geometry->numIndices = static_cast<uint32_t>(indices.size());
		geometry->numVertices = static_cast<uint32_t>(positions.size());

		m_MeshInfo = std::make_shared<engine::MeshInfo>();
		m_MeshInfo->name = "CubeMesh";
		m_MeshInfo->buffers = m_Buffers;
		m_MeshInfo->objectSpaceBounds = box3(float3(-0.5f), float3(0.5f));
		m_MeshInfo->totalIndices = geometry->numIndices;
		m_MeshInfo->totalVertices = geometry->numVertices;
		m_MeshInfo->geometries.push_back(geometry);
	}
}
