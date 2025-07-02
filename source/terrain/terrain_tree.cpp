#include "terrain_tree.h"

#include "donut/shaders/bindless.h"

using namespace donut;
using namespace donut::math;


static void CreateTerrainMeshData(
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


TerrainTile::TerrainTile(
	donut::engine::SceneGraph* sceneGraph,
	const std::shared_ptr<donut::engine::SceneGraphNode>& parent, 
	std::shared_ptr<donut::engine::MeshInfo> terrainMesh, 
	uint level,
	uint tileIndex,
	uint parentIndex
)
	: m_Level(level)
	, m_TileIndex(tileIndex)
	, m_ParentIndex(parentIndex)
{
	std::fill(m_ChildrenIndices.begin(), m_ChildrenIndices.end(), InvalidIndex);

	m_Node = std::make_shared<engine::SceneGraphNode>();
	sceneGraph->Attach(parent, m_Node);

	m_MeshInstance = std::make_shared<TerrainMeshInstance>(std::move(terrainMesh), m_TileIndex);
	m_Node->SetLeaf(m_MeshInstance);
}


Terrain::Terrain(const CreateParams& params)
	: m_HeightmapExtents(params.HeightmapExtents)
	, m_HeightmapResolution(params.HeightmapResolution)
	, m_HeightmapMetersPerPixel(params.HeightmapExtents / static_cast<float2>(m_HeightmapResolution))
	, m_TerrainResolution(params.TerrainResolution)
	, m_NumLevels(-1)
{
	// Calculate the maximum number of tiles needed to express the entire terrain at the highest level of detail
	m_NumTiles = 0;
	m_NumLevels = 0;

	if (params.FitNumLevelsToHeightmapResolution)
	{
		uint tilesInLevel = 1;
		uint verticesAlongEdge = max(m_TerrainResolution.x, m_TerrainResolution.y);
		while (all(verticesAlongEdge <= m_HeightmapResolution))
		{
			m_NumTiles += tilesInLevel;
			m_NumLevels++;

			tilesInLevel *= 4;
			verticesAlongEdge *= 2;
		}
	}
	else
	{
		uint tilesInLevel = 1;
		m_NumLevels = params.NumLevelsOverride;
		for (uint i = 0; i < m_NumLevels; i++)
		{
			m_NumTiles += tilesInLevel;
			tilesInLevel *= 4;
		}
	}
}

void Terrain::Init(nvrhi::IDevice* device, nvrhi::ICommandList* commandList, engine::SceneGraph* sceneGraph)
{
	// Create mesh
	CreateMesh(device, commandList, m_NumTiles);

	// Create root node in scene graph for terrain tile hierarchy to attach to
	m_TerrainRootNode = std::make_shared<engine::SceneGraphNode>();
	sceneGraph->Attach(sceneGraph->GetRootNode(), m_TerrainRootNode);

	m_Tiles.reserve(m_NumTiles);

	std::vector<InstanceData> tileInstanceData;
	tileInstanceData.reserve(m_NumTiles);

	{
		// Recursively populate the tree
		float3 scale = { m_HeightmapExtents.x, 1, m_HeightmapExtents.y };
		float3 offset = { 0, 0, 0 };
		CreateSubtreeFor(sceneGraph, tileInstanceData, nullptr, m_NumLevels - 1, scale, offset);
	}

	// Finally copy instance data into the instance buffer
	{
		commandList->beginTrackingBufferState(m_Buffers->instanceBuffer, nvrhi::ResourceStates::CopyDest);
		commandList->writeBuffer(m_Buffers->instanceBuffer, tileInstanceData.data(), tileInstanceData.size() * sizeof(InstanceData));
		commandList->setPermanentBufferState(m_Buffers->instanceBuffer, nvrhi::ResourceStates::ShaderResource);
	}
}

void Terrain::CreateMesh(nvrhi::IDevice* device, nvrhi::ICommandList* commandList, uint tileCount)
{
	m_Buffers = std::make_shared<engine::BufferGroup>();

	std::vector<float3> positions;
	std::vector<uint32_t> indices;

	CreateTerrainMeshData(m_TerrainResolution, positions, indices);

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
	// This will be populated later once we have set up all the tiles
	{
		nvrhi::BufferDesc instanceBufferDesc;
		instanceBufferDesc.byteSize = tileCount * sizeof(InstanceData);
		instanceBufferDesc.canHaveRawViews = true;
		instanceBufferDesc.structStride = sizeof(InstanceData);
		instanceBufferDesc.debugName = "TerrainIndexBuffer";
		instanceBufferDesc.initialState = nvrhi::ResourceStates::CopyDest;
		m_Buffers->instanceBuffer = device->createBuffer(instanceBufferDesc);
	}

	// Create mesh info
	{
		auto geometry = std::make_shared<engine::MeshGeometry>();
		geometry->material = nullptr;
		geometry->numIndices = static_cast<uint32_t>(indices.size());
		geometry->numVertices = static_cast<uint32_t>(positions.size());

		m_TerrainMeshInfo = std::make_shared<engine::MeshInfo>();
		m_TerrainMeshInfo->name = "TerrainMesh";
		m_TerrainMeshInfo->buffers = m_Buffers;
		m_TerrainMeshInfo->objectSpaceBounds = box3(float3(-0.5f), float3(0.5f));
		m_TerrainMeshInfo->totalIndices = geometry->numIndices;
		m_TerrainMeshInfo->totalVertices = geometry->numVertices;
		m_TerrainMeshInfo->geometries.push_back(geometry);
	}
}

uint Terrain::CreateSubtreeFor(
	donut::engine::SceneGraph* sceneGraph,
	std::vector<InstanceData>& instanceData,
	TerrainTile* parent,
	uint level,
	float3 scale,
	float3 offset
)
{
	// Create tile instance
	uint tileIndex = static_cast<uint>(m_Tiles.size());
	m_Tiles.emplace_back(std::make_shared<TerrainTile>(
		sceneGraph,
		parent ? parent->GetGraphNode() : m_TerrainRootNode,
		m_TerrainMeshInfo,
		level,
		tileIndex,
		parent ? parent->GetTileIndex() : TerrainTile::InvalidIndex
	));
	TerrainTile* tile = m_Tiles.back().get();

	// Calculate tile transform and add it to instance data
	InstanceData& iData = instanceData.emplace_back();
	iData.transform = float3x4(transpose(affineToHomogeneous(scaling(scale) * translation(offset))));
	iData.prevTransform = iData.transform;

	// Recursively create all children
	if (level > 0)
	{
		// Compute transforms for children
		float3 halfScale = scale * float3{ 0.5f, 1.0f, 0.5f };
		float3 offsets[4] = {
			offset + 0.5f * halfScale * float3{ -1, 0, -1 },
			offset + 0.5f * halfScale * float3{  1, 0, -1 },
			offset + 0.5f * halfScale * float3{ -1, 0,  1 },
			offset + 0.5f * halfScale * float3{  1, 0,  1 }
		};

		// Create the 4 children for this node
		tile->SetChildIndex(0, CreateSubtreeFor(sceneGraph, instanceData, tile, level - 1, halfScale, offsets[0]));
		tile->SetChildIndex(1, CreateSubtreeFor(sceneGraph, instanceData, tile, level - 1, halfScale, offsets[1]));
		tile->SetChildIndex(2, CreateSubtreeFor(sceneGraph, instanceData, tile, level - 1, halfScale, offsets[2]));
		tile->SetChildIndex(3, CreateSubtreeFor(sceneGraph, instanceData, tile, level - 1, halfScale, offsets[3]));
	}

	return tileIndex;
}


void Terrain::GetAllTilesAtLevel(uint level, std::vector<TerrainTile*>& outTiles) const
{
	assert(level < m_NumLevels);
	outTiles.clear();

	uint tileCount = static_cast<uint>(pow(4, (m_NumLevels - 1) - level));
	outTiles.reserve(tileCount);

	GetAllTilesAtLevel_Impl(0, level, outTiles);
}

void Terrain::GetAllTilesAtLevel_Impl(uint nodeIndex, uint level, std::vector<TerrainTile*>& outTiles) const
{
	const auto& tile = m_Tiles.at(nodeIndex);
	if (tile->GetLevel() == level)
	{
		outTiles.push_back(tile.get());
	}
	else if (tile->HasChildren())
	{
		GetAllTilesAtLevel_Impl(tile->GetChildIndex(0), level, outTiles);
		GetAllTilesAtLevel_Impl(tile->GetChildIndex(1), level, outTiles);
		GetAllTilesAtLevel_Impl(tile->GetChildIndex(2), level, outTiles);
		GetAllTilesAtLevel_Impl(tile->GetChildIndex(3), level, outTiles);
	}
}
