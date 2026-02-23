// FrustumCull.hlsl — Per-chunk GPU frustum culling
// Writes DrawIndexedInstancedIndirect args into a shared RWByteAddressBuffer.
// Culled chunks get IndexCount=0 so the GPU skips them automatically.

struct ChunkCullData {
    float3 aabbMin;
    uint   drawArgsOffset;   // byte offset into SharedDrawArgs
    float3 aabbMax;
    uint   faceCount;        // from mesh generation counter
};

cbuffer CullConstants : register(b0) {
    float4 FrustumPlanes[6];
    uint   ChunkCount;
    uint3  _pad;
};

StructuredBuffer<ChunkCullData> Chunks      : register(t0);
RWByteAddressBuffer             SharedDrawArgs : register(u0);

// Returns true if the AABB is entirely outside a single plane.
bool IsOutsidePlane(float4 plane, float3 aabbMin, float3 aabbMax)
{
    float3 pVertex;
    pVertex.x = (plane.x >= 0) ? aabbMax.x : aabbMin.x;
    pVertex.y = (plane.y >= 0) ? aabbMax.y : aabbMin.y;
    pVertex.z = (plane.z >= 0) ? aabbMax.z : aabbMin.z;
    return dot(plane.xyz, pVertex) + plane.w < 0;
}

bool IsChunkVisible(float3 aabbMin, float3 aabbMax)
{
    [unroll]
    for (uint i = 0; i < 6; i++) {
        if (IsOutsidePlane(FrustumPlanes[i], aabbMin, aabbMax))
            return false;
    }
    return true;
}

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint idx = dtid.x;
    if (idx >= ChunkCount) return;

    ChunkCullData chunk = Chunks[idx];
    uint offset = chunk.drawArgsOffset;

    if (IsChunkVisible(chunk.aabbMin, chunk.aabbMax) && chunk.faceCount > 0)
    {
        uint indexCount = chunk.faceCount * 6;

        SharedDrawArgs.Store(offset +  0, indexCount);  // IndexCountPerInstance
        SharedDrawArgs.Store(offset +  4, 1);           // InstanceCount
        SharedDrawArgs.Store(offset +  8, 0);           // StartIndexLocation
        SharedDrawArgs.Store(offset + 12, 0);           // BaseVertexLocation
        SharedDrawArgs.Store(offset + 16, 0);           // StartInstanceLocation
    }
    else
    {
        SharedDrawArgs.Store(offset +  0, 0);
        SharedDrawArgs.Store(offset +  4, 0);
        SharedDrawArgs.Store(offset +  8, 0);
        SharedDrawArgs.Store(offset + 12, 0);
        SharedDrawArgs.Store(offset + 16, 0);
    }
}
