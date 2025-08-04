#pragma once

#include <donut/engine/View.h>

struct PlanarViewExConstants;


// Custom view type to extend Donut's PlanarView
class PlanarViewEx : public donut::engine::PlanarView
{
public:
	virtual void FillPlanarViewExConstants(PlanarViewExConstants& constants) const;

	[[nodiscard]] inline size_t GetTerrainViewIndex() const { return m_TerrainViewIndex; }

	inline void SetTerrainViewIndex(size_t index) { m_TerrainViewIndex = index; }

protected:
	size_t m_TerrainViewIndex = 0;
};
