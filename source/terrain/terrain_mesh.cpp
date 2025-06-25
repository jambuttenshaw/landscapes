#include "terrain_mesh.h"

using namespace donut;
using namespace donut::math;


void landscapes::CreateTerrainMesh(
	uint2 resolution,
	std::vector<float2>& outVertices,
	std::vector<uint32_t>& outIndices
)
{
	const float2 fRes = static_cast<float2>(resolution);

	outVertices.clear();
	outVertices.resize(static_cast<size_t>(resolution.x * resolution.y));

	outIndices.clear();
	outIndices.resize(static_cast<size_t>(6 * (resolution.x - 1) * (resolution.y - 1)));

	uint vertex = 0;
	for (uint x = 0; x < resolution.x; x++)
	{
		for (uint y = 0; y < resolution.y; y++)
		{
			outVertices[vertex++] = {
				static_cast<float>(x) / (fRes.x - 1.0f),
				static_cast<float>(y) / (fRes.y - 1.0f),
			};
		}
	}

	uint index = 0;
	for (uint x = 0; x < resolution.x - 1; x++)
	{
		for (uint y = 0; y < resolution.y - 1; y++)
		{
			outIndices[index + 0] = (y + 0) + resolution.x * (x + 0);
			outIndices[index + 1] = (y + 0) + resolution.x * (x + 1);
			outIndices[index + 2] = (y + 1) + resolution.x * (x + 1);

			outIndices[index + 3] = (y + 0) + resolution.x * (x + 0);
			outIndices[index + 4] = (y + 1) + resolution.x * (x + 1);
			outIndices[index + 5] = (y + 1) + resolution.x * (x + 0);

			index += 6;
		}
	}
}
