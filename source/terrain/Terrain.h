#pragma once

#include <nvrhi/nvrhi.h>
#include <donut/core/math/math.h>
#include <donut/engine/SceneGraph.h>

#include "donut/engine/TextureCache.h"

using namespace donut::math;

// Primary = camera view
// Secondary = e.g. view from light source
// Each view requires its own tessellation scheme and mesh
enum TerrainViewType : uint8_t
{
	TerrainViewType_Primary = 0,
	TerrainViewType_Secondary,
	TerrainViewType_COUNT
};


class TerrainView
{
public:
	explicit TerrainView(TerrainViewType viewType, uint maxDepth);

	void Init(nvrhi::IDevice* device, nvrhi::ICommandList* commandList);

	inline nvrhi::IBuffer* GetCBTBuffer() const { return m_CBTBuffer; }

protected:
	TerrainViewType m_ViewType;

	uint m_MaxDepth = 8;
	nvrhi::BufferHandle m_CBTBuffer;
};


class Terrain
{
public:
	struct CreateParams
	{
		// Heightmap parameters
		float2 HeightmapExtents = float2{ 512, 512 };
		uint2 HeightmapResolution = uint2{ 256, 256 };
		float HeightmapHeightScale = 100.0f;
		std::filesystem::path HeightmapTexturePath;

		// Terrain mesh parameters
		uint CBTMaxDepth = 8;
		uint CBTInitDepth = 1;
	};

	bool Init(
		nvrhi::IDevice* device,
		nvrhi::ICommandList* commandList,
		donut::engine::TextureCache* textureCache,
		donut::engine::SceneGraph* sceneGraph,
		const CreateParams& params
	);

	inline float GetHeightScale() const { return m_HeightmapHeightScale; }
	inline void SetHeightScale(float scale) { m_HeightmapHeightScale = scale; }

	inline const TerrainView& GetTerrainView(TerrainViewType viewType) const { return m_TerrainViews.at(viewType); }

	// Get data for rendering
	void FillTerrainConstants(struct TerrainConstants& terrainConstants) const;
	nvrhi::TextureHandle GetHeightmapTexture() const { return m_HeightmapTexture; }

private:
	// The width and height of the entire terrain
	float2 m_HeightmapExtents;
	float m_HeightmapHeightScale = 0.;
	// The number of pixels in the heightmap
	uint2 m_HeightmapResolution;
	// The number of meters each pixel in the heightmap corresponds to
	float2 m_HeightmapMetersPerPixel;

	// The terrain effectively requires a different mesh for different types of view as different views will use different tessellation schemes
	std::vector<TerrainView> m_TerrainViews;

	// Textures
	nvrhi::TextureHandle m_HeightmapTexture;

	// Scene graph objects
	std::shared_ptr<donut::engine::SceneGraphNode> m_TerrainRootNode;
};
