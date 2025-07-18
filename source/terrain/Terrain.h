#pragma once

#include <nvrhi/nvrhi.h>
#include <donut/core/math/math.h>
#include <donut/engine/SceneGraph.h>

#include "donut/engine/TextureCache.h"

using namespace donut::math;


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

	~Terrain();

	bool Init(
		nvrhi::IDevice* device,
		nvrhi::ICommandList* commandList,
		donut::engine::TextureCache* textureCache,
		donut::engine::SceneGraph* sceneGraph,
		const CreateParams& params
	);

	inline float GetHeightScale() const { return m_HeightmapHeightScale; }
	inline void SetHeightScale(float scale) { m_HeightmapHeightScale = scale; }

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

	// CBT
	uint m_CBTMaxDepth = 8;
	nvrhi::BufferHandle m_CBTBuffer;

	// Textures
	nvrhi::TextureHandle m_HeightmapTexture;

	// Scene graph objects
	std::shared_ptr<donut::engine::SceneGraphNode> m_TerrainRootNode;
};
