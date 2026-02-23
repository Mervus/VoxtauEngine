// Shadow Raytrace Compute Shader
// Performs DDA voxel raytracing on GPU for per-pixel shadow computation

// Voxel data (3D texture)
Texture3D<uint> voxelWorld : register(t0);

// Depth buffer from scene
Texture2D<float> depthBuffer : register(t1);

// Output shadow map (1.0 = lit, 0.0 = in shadow)
RWTexture2D<float> shadowMap : register(u0);

// Point sampler for depth buffer
SamplerState pointSampler : register(s0);

cbuffer ShadowConstants : register(b0)
{
    float3 sunDirection;      // Normalized direction towards sun
    float maxRayDistance;     // Maximum ray distance for shadow testing

    float3 worldMinBounds;    // Minimum world bounds (typically 0,0,0)
    float voxelSize;          // Size of each voxel (typically 1.0)

    float3 worldMaxBounds;    // Maximum world bounds (worldSize in voxels)
    float shadowBias;         // Small offset to prevent self-shadowing

    matrix inverseViewProj;   // Inverse view-projection matrix

    float2 screenSize;        // Screen dimensions
    float maxShadowDistance;  // Max distance from camera to compute shadows
    float padding;

    float3 cameraPosition;    // Camera world position
    float padding2;

    float3 voxelWorldOffset;  // World-space origin of the GPU voxel texture
    float padding3;
};

// Reconstruct world position from screen coordinates and depth
float3 ReconstructWorldPosition(uint2 screenPos, float depth)
{
    // Convert screen position to NDC (-1 to 1)
    float2 ndc;
    ndc.x = (screenPos.x / screenSize.x) * 2.0 - 1.0;
    ndc.y = 1.0 - (screenPos.y / screenSize.y) * 2.0;  // Flip Y

    // Create clip space position
    float4 clipPos = float4(ndc.x, ndc.y, depth, 1.0);

    // Transform to world space
    float4 worldPos = mul(clipPos, inverseViewProj);
    worldPos /= worldPos.w;

    return worldPos.xyz;
}

// Check if a world position is inside the GPU voxel texture bounds
bool IsInsideWorld(int3 worldPos)
{
    int3 texPos = worldPos - (int3)voxelWorldOffset;
    return texPos.x >= 0 && texPos.x < (int)worldMaxBounds.x &&
           texPos.y >= 0 && texPos.y < (int)worldMaxBounds.y &&
           texPos.z >= 0 && texPos.z < (int)worldMaxBounds.z;
}

// Check if voxel at world position is solid (non-air)
bool IsVoxelSolid(int3 worldPos)
{
    // Convert world position to GPU texture position
    int3 texPos = worldPos - (int3)voxelWorldOffset;

    // Check bounds in texture space
    if (texPos.x < 0 || texPos.x >= (int)worldMaxBounds.x ||
        texPos.y < 0 || texPos.y >= (int)worldMaxBounds.y ||
        texPos.z < 0 || texPos.z >= (int)worldMaxBounds.z) {
        return false;
    }

    uint voxelType = voxelWorld.Load(int4(texPos, 0));
    return voxelType > 0;  // 0 = air, anything else is solid
}

// DDA Raytrace algorithm - returns true if ray hits solid voxel (in shadow)
bool CastShadowRay(float3 origin, float3 direction)
{
    // Normalize direction
    float3 dir = normalize(direction);

    // Current voxel position
    int x = (int)floor(origin.x);
    int y = (int)floor(origin.y);
    int z = (int)floor(origin.z);

    // Direction to step in each axis (-1 or 1)
    int stepX = (dir.x > 0) ? 1 : -1;
    int stepY = (dir.y > 0) ? 1 : -1;
    int stepZ = (dir.z > 0) ? 1 : -1;

    // Distance along ray to cross one voxel in each axis
    float tDeltaX = (dir.x != 0) ? abs(1.0 / dir.x) : 1e30;
    float tDeltaY = (dir.y != 0) ? abs(1.0 / dir.y) : 1e30;
    float tDeltaZ = (dir.z != 0) ? abs(1.0 / dir.z) : 1e30;

    // Distance to next voxel boundary in each axis
    float tMaxX, tMaxY, tMaxZ;

    if (dir.x > 0)
        tMaxX = ((float)x + 1.0 - origin.x) / dir.x;
    else if (dir.x < 0)
        tMaxX = (origin.x - (float)x) / -dir.x;
    else
        tMaxX = 1e30;

    if (dir.y > 0)
        tMaxY = ((float)y + 1.0 - origin.y) / dir.y;
    else if (dir.y < 0)
        tMaxY = (origin.y - (float)y) / -dir.y;
    else
        tMaxY = 1e30;

    if (dir.z > 0)
        tMaxZ = ((float)z + 1.0 - origin.z) / dir.z;
    else if (dir.z < 0)
        tMaxZ = (origin.z - (float)z) / -dir.z;
    else
        tMaxZ = 1e30;

    // DDA traversal
    float t = 0.0;
    int maxSteps = (int)maxRayDistance * 3;  // Limit iterations
    bool firstStep = true;  // Skip first voxel to avoid self-shadowing

    [loop]
    for (int i = 0; i < maxSteps && t < maxRayDistance; i++)
    {
        // Check current voxel (skip only the exact starting voxel)
        if (!firstStep && IsVoxelSolid(int3(x, y, z)))
        {
            return true;  // Hit solid voxel - in shadow
        }
        firstStep = false;

        // Step to next voxel boundary
        if (tMaxX < tMaxY && tMaxX < tMaxZ)
        {
            x += stepX;
            t = tMaxX;
            tMaxX += tDeltaX;
        }
        else if (tMaxY < tMaxZ)
        {
            y += stepY;
            t = tMaxY;
            tMaxY += tDeltaY;
        }
        else
        {
            z += stepZ;
            t = tMaxZ;
            tMaxZ += tDeltaZ;
        }

        // Early out if we've left the world
        if (!IsInsideWorld(int3(x, y, z)))
        {
            return false;  // Left world bounds - not in shadow
        }
    }

    return false;  // No hit within max distance - not in shadow
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchThreadID.xy;

    // Check if within screen bounds
    if (pixelCoord.x >= (uint)screenSize.x || pixelCoord.y >= (uint)screenSize.y)
    {
        return;
    }

    // Sample depth buffer
    float depth = depthBuffer.Load(int3(pixelCoord, 0));

    // Sky pixels (depth >= 1.0) are fully lit
    if (depth >= 1.0)
    {
        shadowMap[pixelCoord] = 1.0;
        return;
    }

    // Reconstruct world position from depth
    float3 worldPos = ReconstructWorldPosition(pixelCoord, depth);

    // Skip shadow computation for pixels beyond max shadow distance
    float distToCamera = length(worldPos - cameraPosition);
    if (distToCamera > maxShadowDistance)
    {
        shadowMap[pixelCoord] = 1.0;  // Fully lit (no shadows at distance)
        return;
    }

    // Offset the ray origin slightly along the sun direction to avoid self-shadowing
    float3 rayOrigin = worldPos + sunDirection * shadowBias;

    // Cast shadow ray towards the sun
    bool inShadow = CastShadowRay(rayOrigin, sunDirection);

    // Write result (1.0 = lit, 0.0 = shadow)
    shadowMap[pixelCoord] = inShadow ? 0.0 : 1.0;
}
