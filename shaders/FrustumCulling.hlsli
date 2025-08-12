#ifndef FRUSTUMCULLING_H
#define FRUSTUMCULLING_H


bool FrustumCullingTest0(const float4 planes[6], float3 bmin, float3 bmax)
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

bool FrustumCullingTest(const float4 planes[6], float3 bmin, float3 bmax)
{
    for (int i = 0; i < 6; i++)
    {
        int test = 0;
        test += dot(planes[i], float4(bmin.x, bmin.y, bmin.z, 1.0f)) < 0.0;
        test += dot(planes[i], float4(bmax.x, bmin.y, bmin.z, 1.0f)) < 0.0;
        test += dot(planes[i], float4(bmin.x, bmax.y, bmin.z, 1.0f)) < 0.0;
        test += dot(planes[i], float4(bmax.x, bmax.y, bmin.z, 1.0f)) < 0.0;
        test += dot(planes[i], float4(bmin.x, bmin.y, bmax.z, 1.0f)) < 0.0;
        test += dot(planes[i], float4(bmax.x, bmin.y, bmax.z, 1.0f)) < 0.0;
        test += dot(planes[i], float4(bmin.x, bmax.y, bmax.z, 1.0f)) < 0.0;
        test += dot(planes[i], float4(bmax.x, bmax.y, bmax.z, 1.0f)) < 0.0;

        if (test == 8)
            return false;
    }

    return true;
}

#endif