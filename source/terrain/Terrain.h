#pragma once

#include <nvrhi/nvrhi.h>

#include <donut/core/math/math.h>
#include <donut/engine/SceneGraph.h>
#include <donut/engine/TextureCache.h>

using namespace donut::math;

// Primary = camera view
// Secondary = e.g. view from light source
// Each view requires its own tessellation scheme and therefore own mesh
enum TerrainViewType : uint8_t
{
	TerrainViewType_Primary = 0,
	TerrainViewType_Secondary,
	TerrainViewType_Count
};


class TerrainMeshInfo;

class TerrainMeshView
{
public:
	explicit TerrainMeshView(const TerrainMeshInfo* parent, TerrainViewType viewType, uint maxDepth, uint initDepth);

	void Init(nvrhi::IDevice* device, nvrhi::ICommandList* commandList);

	[[nodiscard]] inline const TerrainMeshInfo* GetParent() const { return m_Parent; }
	[[nodiscard]] inline nvrhi::IBuffer* GetCBTBuffer() const { return m_CBTBuffer; }
	[[nodiscard]] inline uint GetMaxDepth() const { return m_MaxDepth; }

	[[nodiscard]] inline nvrhi::IBuffer* GetIndirectArgsBuffer() const { return m_IndirectArgsBuffer; }
	[[nodiscard]] inline uint GetIndirectArgsDispatchOffset() const { return 0; }
	[[nodiscard]] inline uint GetIndirectArgsDrawOffset() const { return sizeof(nvrhi::DispatchIndirectArguments); }

protected:
	const TerrainMeshInfo* m_Parent;
	TerrainViewType m_ViewType;

	uint m_MaxDepth = 8;
	uint m_InitDepth = 1;
	nvrhi::BufferHandle m_CBTBuffer;
	nvrhi::BufferHandle m_IndirectArgsBuffer;
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

		// terrain mesh parameters
		uint CBTMaxDepth = 8;
		uint CBTInitDepth = 1;
	};

	TerrainMeshInfo(
		nvrhi::IDevice* device,
		nvrhi::ICommandList* commandList,
		donut::engine::TextureCache* textureCache,
		const CreateParams& params
	);

	inline void SetHeightScale(float scale) { m_HeightmapHeightScale = scale; }

	[[nodiscard]] inline float2 GetExtents() const { return m_HeightmapExtents; }
	[[nodiscard]] inline float GetHeightScale() const { return m_HeightmapHeightScale; }

	[[nodiscard]] inline size_t GetNumTerrainViews() const { return m_TerrainViews.size(); }
	[[nodiscard]] inline const TerrainMeshView* GetTerrainView(size_t viewIndex) const { return &m_TerrainViews.at(viewIndex); }

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
	std::vector<TerrainMeshView> m_TerrainViews;

	// GPU resources
	nvrhi::TextureHandle m_HeightmapTexture;
	nvrhi::BufferHandle m_TerrainCB;
};


class TerrainMeshInstance : public donut::engine::MeshInstance
{
public:
	explicit TerrainMeshInstance(std::shared_ptr<TerrainMeshInfo> terrain);

	[[nodiscard]] dm::box3 GetLocalBoundingBox() override;
	[[nodiscard]] std::shared_ptr<SceneGraphLeaf> Clone() override;
	[[nodiscard]] donut::engine::SceneContentFlags GetContentFlags() const override;

	[[nodiscard]] TerrainMeshInfo* GetTerrain() const { return dynamic_cast<TerrainMeshInfo*>(m_Mesh.get()); }

protected:
	// Shorthand for internal use
	TerrainMeshInfo& Terrain() const { return dynamic_cast<TerrainMeshInfo&>(*m_Mesh); }
};
