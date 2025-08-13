#ifndef FRUSTUMCULLING_H
#define FRUSTUMCULLING_H


// Returns true if any part of the bounding box is within the frustum
bool FrustumCullingTest(const float4 planes[6], float3 bmin, float3 bmax)
{
    for (int i = 0; i < 6; ++i)
    {
		float4 plane = planes[i];
		float3 p = select(plane.xyz >= 0, bmin, bmax);

		if (dot(p, plane.xyz) > plane.w)
            return false;
	}

    return true;
}

bool FrustumCullingTest(const float4 planes[6], float3 patchVertices_WorldSpace[3])
{
	float3 bmin = min(min(patchVertices_WorldSpace[0], patchVertices_WorldSpace[1]), patchVertices_WorldSpace[2]);
	float3 bmax = max(max(patchVertices_WorldSpace[0], patchVertices_WorldSpace[1]), patchVertices_WorldSpace[2]);

	return FrustumCullingTest(planes, bmin, bmax);
}

#endif