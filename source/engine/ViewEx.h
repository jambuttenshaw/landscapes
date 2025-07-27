#pragma once

#include <donut/engine/View.h>


// Custom view type to extend Donut's PlanarView
class PlanarViewEx : public donut::engine::PlanarView
{
public:

	inline void SetTerrainViewIndex(size_t index) { m_TerrainViewIndex = index; }

	[[nodiscard]] inline size_t GetTerrainViewIndex() const { return m_TerrainViewIndex; }

protected:
	size_t m_TerrainViewIndex = 0;
};
