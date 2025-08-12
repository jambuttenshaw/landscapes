#ifndef FRUSTUMCULLING_H
#define FRUSTUMCULLING_H


bool FrustumCullingTest(const float4 planes[6], float3 bmin, float3 bmax)
{
    for (int i = 0; i < 6; ++i)
    {
		float4 plane = planes[i];
		float3 p = select(bmin, bmax, plane.xyz > 0);
            
		if (dot(p, plane.xyz) > plane.w)
            return false;
	}

    return true;
}

#endif