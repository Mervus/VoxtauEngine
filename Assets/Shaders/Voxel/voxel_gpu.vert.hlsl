// voxel_gpu.vert.hlsl — Vertex shader for GPU-meshed chunks
// Reconstructs 4 corner vertices from 2 packed face uints (produced by ChunkMesh.hlsl)
// uint[0] = position + face + texIndex
// uint[1] = 4 corner AO values (2 bits each)

// Frame constants (slot 0)
cbuffer PerFrameBuffer : register(b0)
{
    float4x4 ViewProjection;
    float3   CameraPosition;
    float    _pad0;
};

// Per-object constants (slot 1) - chunk world position as integer
cbuffer ChunkPositionBuffer : register(b1)
{
    int3  ChunkWorldPos;
    float VoxelScale;
};

// Packed face data from compute shader — 2 uints per face
StructuredBuffer<uint> FaceBuffer : register(t0);

// Output to pixel shader
struct VSOutput
{
    float4 position  : SV_POSITION;
    float3 worldPos  : WORLDPOS;
    float3 normal    : NORMAL;
    float2 uv        : TEXCOORD0;
    uint   blockType : BLOCKTYPE;
    float  ao        : AMBIENT_OCCLUSION;
};

// Face normals lookup (matches compute shader)
static const float3 Normals[6] = {
    float3( 1,  0,  0),  // 0: +X
    float3(-1,  0,  0),  // 1: -X
    float3( 0,  1,  0),  // 2: +Y
    float3( 0, -1,  0),  // 3: -Y
    float3( 0,  0,  1),  // 4: +Z
    float3( 0,  0, -1),  // 5: -Z
};

// Corner offsets for each face (4 corners x 6 faces = 24 entries)
// Each entry is a 3-bit packed xyz offset (0 or 1 per axis)
// Encoding: bit0=x, bit1=y, bit2=z  -> value = x | (y<<1) | (z<<2)
//   0=(0,0,0) 1=(1,0,0) 2=(0,1,0) 3=(1,1,0)
//   4=(0,0,1) 5=(1,0,1) 6=(0,1,1) 7=(1,1,1)
static const uint CornerLUT[24] = {
    // +X: (1,0,0),(1,1,0),(1,1,1),(1,0,1)
    1, 3, 7, 5,
    // -X: (0,0,1),(0,1,1),(0,1,0),(0,0,0)
    4, 6, 2, 0,
    // +Y: (0,1,0),(0,1,1),(1,1,1),(1,1,0)
    2, 6, 7, 3,
    // -Y: (0,0,1),(0,0,0),(1,0,0),(1,0,1)
    4, 0, 1, 5,
    // +Z: (0,0,1),(1,0,1),(1,1,1),(0,1,1)
    4, 5, 7, 6,
    // -Z: (1,0,0),(0,0,0),(0,1,0),(1,1,0)
    1, 0, 2, 3,
};

// UV coordinates per corner index (same for all faces)
static const float2 UVs[4] = {
    float2(0, 0),
    float2(0, 1),
    float2(1, 1),
    float2(1, 0),
};

// AO value to float lookup: 0 -> darkest, 3 -> brightest
// Using a curve that looks good for voxel games
static const float AOTable[4] = {
    0.2,   // AO = 0: fully occluded (both edges solid)
    0.5,   // AO = 1: mostly occluded
    0.75,  // AO = 2: slightly occluded
    1.0    // AO = 3: fully lit (no occlusion)
};

VSOutput main(uint vertexID : SV_VertexID)
{
    // Each face has 4 vertices
    uint faceIdx   = vertexID / 4;
    uint cornerIdx = vertexID % 4;

    // Read 2 packed uints per face
    uint packed = FaceBuffer[faceIdx * 2 + 0];
    uint aoData = FaceBuffer[faceIdx * 2 + 1];
    uint flipQuad = (aoData >> 8) & 0x1;
    uint adjustedCorner = flipQuad ? ((cornerIdx + 1) % 4) : cornerIdx;

    // Unpack geometry: x[4:0], y[9:5], z[14:10], face[17:15], texIndex[25:18]
    float3 blockPos;
    blockPos.x = float(packed & 0x1F);
    blockPos.y = float((packed >> 5) & 0x1F);
    blockPos.z = float((packed >> 10) & 0x1F);

    uint faceId   = (packed >> 15) & 0x7;
    uint texIndex = (packed >> 18) & 0xFF;

    // Unpack AO for this corner: each corner uses 2 bits
    // corner0[1:0], corner1[3:2], corner2[5:4], corner3[7:6]
    uint aoValue = (aoData >> (adjustedCorner * 2)) & 0x3;
    float ao = AOTable[aoValue];

    // Look up corner offset from LUT
    uint cornerBits = CornerLUT[faceId * 4 + adjustedCorner];
    float3 cornerOffset;
    cornerOffset.x = float(cornerBits & 1);
    cornerOffset.y = float((cornerBits >> 1) & 1);
    cornerOffset.z = float((cornerBits >> 2) & 1);

    // World position = (chunk origin + block position + corner offset) * scale
    float3 worldPos = (float3(ChunkWorldPos) + blockPos + cornerOffset) * VoxelScale;

    // Transform to clip space
    float4 clipPos = mul(float4(worldPos, 1.0), ViewProjection);

    VSOutput output;
    output.position  = clipPos;
    output.worldPos  = worldPos;
    output.normal    = Normals[faceId];
    output.uv        = UVs[adjustedCorner];
    output.blockType = texIndex;
    output.ao        = ao;

    return output;
}