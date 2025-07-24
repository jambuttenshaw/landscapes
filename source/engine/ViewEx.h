#pragma once

#include <donut/engine/View.h>


// Custom view type to extend Donut's PlanarView
class PlanarViewEx : public donut::engine::PlanarView
{
public:

	inline void SetLightView(bool isLightView) { m_IsLightView = isLightView; }

	[[nodiscard]] inline bool IsPrimaryView() const { return !m_IsLightView; }
	[[nodiscard]] inline bool IsLightView() const { return m_IsLightView; }

protected:
	bool m_IsLightView = false;
};
