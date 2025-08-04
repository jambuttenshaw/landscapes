#include "ViewEx.h"

using namespace donut::math;

#include "ViewEx_cb.h"


void PlanarViewEx::FillPlanarViewExConstants(PlanarViewExConstants& constants) const
{
	// view frustum
	for (uint32_t p = 0; p < 6; p++)
	{
		const auto& plane = m_ViewFrustum.planes[p];
		constants.viewFrustum[p] = float4(plane.normal, plane.distance);
	}
}
