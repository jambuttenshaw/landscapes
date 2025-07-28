
#include <donut/shaders/binding_helpers.hlsli>
#include "TerrainShaders.h"

#define CBT_HEAP_BUFFER_BINDING REGISTER_SRV(TESSELLATION_BINDING_CBT, TESSELLATION_SPACE_CBT)
#include "ConcurrentBinaryTree.hlsl"

struct IndirectArgs
{
    struct
    {
        uint groupsX;
        uint groupsY;
        uint groupsZ;
    } cbtDispatch;

    struct
    {
        uint vertexCount;
        uint instanceCount;
        uint startVertexLocation;
        uint startInstanceLocation;
    } lebDispatch;
};
RWStructuredBuffer<IndirectArgs> RWIndirectArgs : REGISTER_UAV(TESSELLATION_BINDING_INDIRECT_ARGS, TESSELLATION_SPACE_CBT);

[numthreads(1, 1, 1)]
void cbt_dispatcher_cs()
{
    uint nodeCount = cbt_NodeCount();
    RWIndirectArgs[0].cbtDispatch.groupsX = max(nodeCount >> 8, 1);
}

[numthreads(1, 1, 1)]
void leb_dispatcher_cs()
{
    RWIndirectArgs[0].lebDispatch.instanceCount = cbt_NodeCount();
}
