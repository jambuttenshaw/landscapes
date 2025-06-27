#pragma once

#include <vector>

#include <donut/core/math/math.h>


namespace landscapes
{

	void CreateTerrainMesh(
		donut::math::uint2 resolution,
		std::vector<donut::math::float3>& outVertices,
		std::vector<uint32_t>& outIndices
	);

}
