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

	void Init(nvrhi::IDevice* device, nvrhi::ICommandList* commandList);

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


class TerrainMeshInfo : public donut::engine::MeshInfo
{
public:
	
	struct CreateParams
	{
		// Heightmap parameters
		float2 HeightmapExtents = float2{ 512, 512 };
		uint2 HeightmapResolution = uint2{ 256, 256 };
		float HeightmapHeightScale = 100.0f;
		std::filesystem::path HeightmapTexturePath;

		// Mesh view descriptions
		std::vector<TerrainMeshViewDesc> Views;
	};

	TerrainMeshInfo(
		nvrhi::IDevice* device,
		nvrhi::ICommandList* commandList,
		donut::engine::TextureCache* textureCache,
		const CreateParams& params
	);

	[[nodiscard]] inline float2 GetExtents() const { return m_HeightmapExtents; }
	[[nodiscard]] inline float GetHeightScale() const { return m_HeightmapHeightScale; }

	[[nodiscard]] inline size_t GetNumTerrainViews() const { return m_TerrainViews.size(); }
	[[nodiscard]] inline const TerrainMeshViewDesc& GetTerrainViewDesc(size_t viewIndex) const { return m_TerrainViews.at(viewIndex); }

	[[nodiscard]] nvrhi::TextureHandle GetHeightmapTexture() const { return m_HeightmapTexture; }
	[[nodiscard]] nvrhi::BufferHandle GetConstantBuffer() const { return m_TerrainCB; }

private:
	// The width and height of the entire terrain in real-world units, e.g. meters
	float2 m_HeightmapExtents;
	float m_HeightmapHeightScale = 1.;
	// The number of pixels in the heightmap
	uint2 m_HeightmapResolution;
	// The number of meters each pixel in the heightmap corresponds to
	float2 m_HeightmapMetersPerPixel;

	// The terrain effectively requires a different mesh for different types of view as different views will use different tessellation schemes
	// We store the descriptions of the views we want to create
	std::vector<TerrainMeshViewDesc> m_TerrainViews;

	// GPU resources
	nvrhi::TextureHandle m_HeightmapTexture;
	nvrhi::BufferHandle m_TerrainCB;
};


class TerrainMeshInstance : public donut::engine::MeshInstance
{
public:
	explicit TerrainMeshInstance(std::shared_ptr<TerrainMeshInfo> terrain);
	void Init(nvrhi::IDevice* device, nvrhi::ICommandList* commandList);

	[[nodiscard]] dm::box3 GetLocalBoundingBox() override;
	[[nodiscard]] std::shared_ptr<SceneGraphLeaf> Clone() override;
	[[nodiscard]] donut::engine::SceneContentFlags GetContentFlags() const override;

	[[nodiscard]] TerrainMeshInfo* GetTerrain() const { return dynamic_cast<TerrainMeshInfo*>(m_Mesh.get()); }

	[[nodiscard]] inline size_t GetNumTerrainViews() const { return m_TerrainViews.size(); }
	[[nodiscard]] inline const TerrainMeshView* GetTerrainView(size_t viewIndex) const { return &m_TerrainViews.at(viewIndex); }

protected:
	// Shorthand for internal use
	TerrainMeshInfo& Terrain() const { return dynamic_cast<TerrainMeshInfo&>(*m_Mesh); }

protected:
	std::vector<TerrainMeshView> m_TerrainViews;
};
