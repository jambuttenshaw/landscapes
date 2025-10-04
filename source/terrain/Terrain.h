#pragma once

#include <nvrhi/nvrhi.h>

#include <donut/core/math/math.h>
#include <donut/engine/SceneGraph.h>
#include <donut/engine/TextureCache.h>

using namespace donut::math;


class TerrainMeshInstance;
class ITerrainTessellationPass;

struct TerrainMeshViewDesc
{
	uint MaxDepth = 8;
	uint InitDepth = 1;

	// Optional - if null, tessellation of terrain mesh will not be updated
	std::weak_ptr<ITerrainTessellationPass> TessellationScheme;
};

class TerrainMeshView
{
public:
	explicit TerrainMeshView(const TerrainMeshInstance* parent, const TerrainMeshViewDesc& desc);

	void CreateBuffers(nvrhi::IDevice* device, nvrhi::ICommandList* commandList);

	[[nodiscard]] inline const TerrainMeshInstance* GetInstance() const { return m_Instance; }
	[[nodiscard]] inline nvrhi::IBuffer* GetCBTBuffer() const { return m_CBTBuffer; }
	[[nodiscard]] inline uint GetMaxDepth() const { return m_MaxDepth; }

	[[nodiscard]] inline nvrhi::IBuffer* GetIndirectArgsBuffer() const { return m_IndirectArgsBuffer; }

	[[nodiscard]] inline std::weak_ptr<ITerrainTessellationPass> GetTessellationScheme() const { return m_TessellationScheme; }

	[[nodiscard]] inline static uint GetIndirectArgsDispatchOffset() { return 0; }
	[[nodiscard]] inline static uint GetIndirectArgsDrawOffset() { return sizeof(nvrhi::DispatchIndirectArguments); }

protected:
	const TerrainMeshInstance* m_Instance;

	uint m_MaxDepth = 8;
	uint m_InitDepth = 1;
	nvrhi::BufferHandle m_CBTBuffer;
	nvrhi::BufferHandle m_IndirectArgsBuffer;

	std::weak_ptr<ITerrainTessellationPass> m_TessellationScheme;
};


struct TerrainMeshInfo : public donut::engine::MeshInfo
{
	// The width and height of the entire terrain in real-world units, e.g. meters
	float2 HeightmapExtents{ 1.0f, 1.0f };
	float HeightmapHeightScale{ 1.0f };
	// The number of pixels in the heightmap texture (TODO: Could just acquire from texture object?)
	uint2 HeightmapResolution;

	std::filesystem::path HeightmapTexturePath;

	// The terrain effectively requires a different mesh for different types of view as different views will use different tessellation schemes
	// We store the descriptions of the views we want to create
	std::vector<TerrainMeshViewDesc> TerrainViews;

	// GPU resources
	std::shared_ptr<donut::engine::LoadedTexture> HeightmapTexture;
	nvrhi::BufferHandle TerrainCB;
};


class TerrainMeshInstance : public donut::engine::MeshInstance
{
public:
	// Default constructor required when loading from scene graph 
	TerrainMeshInstance();
	// Constructor taking TerrainMeshInfo can be used when programmatically creating terrains
	explicit TerrainMeshInstance(std::shared_ptr<TerrainMeshInfo> terrain);

	virtual void Load(const Json::Value& node) override;

	void CreateBuffers(nvrhi::IDevice* device, nvrhi::ICommandList* commandList);

	[[nodiscard]] dm::box3 GetLocalBoundingBox() override;
	[[nodiscard]] std::shared_ptr<SceneGraphLeaf> Clone() override;
	[[nodiscard]] donut::engine::SceneContentFlags GetContentFlags() const override;

	[[nodiscard]] TerrainMeshInfo* GetTerrain() const { return dynamic_cast<TerrainMeshInfo*>(m_Mesh.get()); }

	[[nodiscard]] inline size_t GetNumTerrainViews() const { return m_TerrainViews.size(); }
	[[nodiscard]] inline const TerrainMeshView* GetTerrainView(size_t viewIndex) const { return &m_TerrainViews.at(viewIndex); }

protected:
	// Shorthand for internal use
	TerrainMeshInfo& Terrain() const { return dynamic_cast<TerrainMeshInfo&>(*m_Mesh); }

private:

	// Called once TerrainMeshInstance has a TerrainMeshInfo
	void Create();

protected:
	std::vector<TerrainMeshView> m_TerrainViews;
};
