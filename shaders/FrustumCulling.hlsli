#ifndef FRUSTUMCULLING_H
#define FRUSTUMCULLING_H

// Frustum Culling test - translated to HLSL from original created by Jonathan Dupuy
// https://github.com/jdupuy/LongestEdgeBisection2D

/**
 * Negative Vertex of an AABB
 *
 * This procedure computes the negative vertex of an AABB
 * given a normal.
 * See the View Frustum Culling tutorial @ LightHouse3D.com
 * http://www.lighthouse3d.com/tutorials/view-frustum-culling/geometric-approach-testing-boxes-ii/
 */
float3 NegativeVertex(float3 bmin, float3 bmax, float3 n)
{
    bool3 b = n > 0;
    return select(bmin, bmax, b);
}

/**
 * Frustum-AABB Culling Test
 *
 * This procedure returns true if the AABB is either inside, or in
 * intersection with the frustum, and false otherwise.
 * The test is based on the View Frustum Culling tutorial @ LightHouse3D.com
 * http://www.lighthouse3d.com/tutorials/view-frustum-culling/geometric-approach-testing-boxes-ii/
 */
bool FrustumCullingTest(const float4 planes[6], float3 bmin, float3 bmax)
{
    float a = 1.0f;

    for (int i = 0; i < 6 && a >= 0.0f; ++i)
    {
		float3 n = NegativeVertex(bmin, bmax, planes[i].xyz);

		a = dot(float4(n, 1.0f), planes[i]);
	}

    return (a >= 0.0);
}

#endif