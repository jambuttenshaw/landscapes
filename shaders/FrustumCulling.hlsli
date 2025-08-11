#ifndef FRUSTUMCULLING_H
#define FRUSTUMCULLING_H

// Frustum Culling test - translated to HLSL from original created by Jonathan Dupuy
// https://github.com/jdupuy/LongestEdgeBisection2D

bool FrustumCullingTest(const float4 planes[6], float3 bmin, float3 bmax)
{
    //for (int i = 0; i < 6; ++i)
    int i = 0;
    {
        float x = planes[i].x > 0 ? bmin.x : bmax.x;
        float y = planes[i].y > 0 ? bmin.y : bmax.y;
        float z = planes[i].z > 0 ? bmin.z : bmax.z;
            
        float distance =
                planes[i].x * x +
                planes[i].y * y +
                planes[i].z * z -
                planes[i].w;

        if (distance > 0.f)
            return false;
	}

    return true;
}

#endif