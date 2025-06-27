#pragma once

#include <donut/core/math/math.h>

#include "donut/engine/SceneGraph.h"
#include "donut/engine/SceneTypes.h"
#include "nvrhi/nvrhi.h"


class TerrainMesh
{
public:
	TerrainMesh(donut::math::uint2 resolution);

	void InitResources(nvrhi::IDevice* device, nvrhi::ICommandList* commandList, std::shared_ptr<donut::engine::Material> material);

	const std::shared_ptr<donut::engine::MeshInfo>& GetMeshInfo() const { return m_MeshInfo; }

protected:
	donut::math::uint2 m_Resolution;

	std::shared_ptr<donut::engine::BufferGroup> m_Buffers;
	std::shared_ptr<donut::engine::MeshInfo> m_MeshInfo;
};
