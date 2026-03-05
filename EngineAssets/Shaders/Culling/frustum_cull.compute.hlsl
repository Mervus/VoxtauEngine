// GPU Frustum Culling Compute Shader
// Performs AABB-frustum testing and LOD selection for voxel chunks

// Input: Chunk metadata (position, LOD mesh info)
struct ChunkGPUData
{
    float3 position;    // World position of chunk corner
    float padding1;
    uint chunkIndex;    // Index for CPU-side mesh lookup
    uint indexCount0;   // Index count for LOD 0
    uint indexCount1;   // Index count for LOD 1
    uint indexCount2;   // Index count for LOD 2
    uint flags;         // Bit 0: has LOD1, Bit 1: has LOD2
    float3 padding2;
};

// Output: Visible chunk indices with LOD level
struct VisibleChunkData
{
    uint chunkIndex;
    uint lodLevel;
    float sortKey;      // Distance for front-to-back sorting
    uint padding;
};

// Chunk metadata buffer (SRV)
StructuredBuffer<ChunkGPUData> chunkData : register(t0);

// Output buffer for visible chunks (append buffer)
AppendStructuredBuffer<VisibleChunkData> visibleChunks : register(u0);

cbuffer CullingConstants : register(b0)
{
    float4 frustumPlanes[6];    // View frustum planes (Ax + By + Cz + D = 0)
    float3 cameraPosition;
    float chunkSize;
    float lodDistance1;         // Switch to LOD 1 beyond this distance
    float lodDistance2;         // Switch to LOD 2 beyond this distance
    float maxRenderDistance;    // Cull chunks beyond this distance
    uint chunkCount;            // Total number of chunks to process
};

// Test if an AABB is outside a single plane
// Returns true if AABB is completely behind (outside) the plane
bool IsAABBOutsidePlane(float3 aabbMin, float3 aabbMax, float4 plane)
{
    // Find the point of the AABB most in the direction of the plane normal
    float3 p;
    p.x = (plane.x >= 0) ? aabbMax.x : aabbMin.x;
    p.y = (plane.y >= 0) ? aabbMax.y : aabbMin.y;
    p.z = (plane.z >= 0) ? aabbMax.z : aabbMin.z;

    // If this point is behind the plane, the entire AABB is outside
    return (dot(plane.xyz, p) + plane.w) < 0;
}

// Test if chunk AABB intersects frustum
bool IsChunkInFrustum(float3 chunkPos)
{
    float3 aabbMin = chunkPos;
    float3 aabbMax = chunkPos + float3(chunkSize, chunkSize, chunkSize);

    // Test against all 6 frustum planes
    [unroll]
    for (int i = 0; i < 6; i++)
    {
        if (IsAABBOutsidePlane(aabbMin, aabbMax, frustumPlanes[i]))
        {
            return false;
        }
    }

    return true;
}

// Calculate LOD level based on distance
uint SelectLOD(float distance, uint flags)
{
    // Check which LODs are available
    bool hasLOD1 = (flags & 0x1) != 0;
    bool hasLOD2 = (flags & 0x2) != 0;

    if (distance > lodDistance2 && hasLOD2)
    {
        return 2;
    }
    else if (distance > lodDistance1 && hasLOD1)
    {
        return 1;
    }

    return 0;
}

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint chunkIdx = dispatchThreadID.x;

    // Early out if beyond chunk count
    if (chunkIdx >= chunkCount)
    {
        return;
    }

    ChunkGPUData chunk = chunkData[chunkIdx];

    // Skip chunks with no data (position at origin with no geometry usually means empty slot)
    if (chunk.indexCount0 == 0)
    {
        return;
    }

    // Calculate distance to chunk center
    float3 chunkCenter = chunk.position + float3(chunkSize * 0.5f, chunkSize * 0.5f, chunkSize * 0.5f);
    float distance = length(chunkCenter - cameraPosition);

    // Distance culling
    if (distance > maxRenderDistance)
    {
        return;
    }

    // Frustum culling
    if (!IsChunkInFrustum(chunk.position))
    {
        return;
    }

    // Select LOD based on distance
    uint lodLevel = SelectLOD(distance, chunk.flags);

    // Verify selected LOD has geometry
    uint indexCount = 0;
    if (lodLevel == 0)
        indexCount = chunk.indexCount0;
    else if (lodLevel == 1)
        indexCount = chunk.indexCount1;
    else
        indexCount = chunk.indexCount2;

    // Fall back to lower LOD if selected LOD has no geometry
    if (indexCount == 0)
    {
        if (lodLevel == 2 && chunk.indexCount1 > 0)
        {
            lodLevel = 1;
        }
        else
        {
            lodLevel = 0;
        }
    }

    // Chunk is visible - append to output
    VisibleChunkData visible;
    visible.chunkIndex = chunk.chunkIndex;
    visible.lodLevel = lodLevel;
    visible.sortKey = distance;
    visible.padding = 0;

    visibleChunks.Append(visible);
}
