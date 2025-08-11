#ifndef DEBUGTOOLS_H
#define DEBUGTOOLS_H

#include <donut/shaders/view_cb.h>


struct DebugPlaneConstants
{
	PlanarViewConstants View;

	float3 PlaneNormal;
	float PlaneSize; // How much of the plane to draw in this debug visualization

	float3 PlaneOrigin;
};

#endif