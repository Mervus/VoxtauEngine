// VOXEL VERTEX SHADER - Compact VoxelVertex format (24 bytes)

// CONSTANT BUFFERS
cbuffer PerFrameBuffer : register(b0)
{
    matrix viewProjection;
    float3 cameraPosition;
    float padding0;
};

cbuffer PerObjectBuffer : register(b1)
{
    matrix world;
    matrix worldInverseTranspose;
};

// Normal lookup table (6 axis-aligned face normals)
static const float3 faceNormals[6] = {
    float3( 0,  0,  1),  // 0: Front  (+Z)
    float3( 0,  0, -1),  // 1: Back   (-Z)
    float3(-1,  0,  0),  // 2: Left   (-X)
    float3( 1,  0,  0),  // 3: Right  (+X)
    float3( 0,  1,  0),  // 4: Top    (+Y)
    float3( 0, -1,  0)   // 5: Bottom (-Y)
};

// INPUT (matches VoxelVertex struct - 24 bytes)
struct VertexInput
{
    float3 position : POSITION;   // 12 bytes
    float2 texCoord : TEXCOORD;   //  8 bytes
    uint   packed   : PACKED;     //  4 bytes
};

// OUTPUT
struct VertexOutput
{
    float4 position      : SV_POSITION;
    float3 worldPosition : WORLD_POS;
    float3 normal        : NORMAL;
    float2 texCoord      : TEXCOORD0;
    float  lightLevel    : LIGHT;
    uint   textureIndex  : TEXINDEX;
    float  fogFactor     : FOG;
};

// VERTEX SHADER MAIN
VertexOutput main(VertexInput input)
{
    VertexOutput output;

    // Unpack the packed uint32:
    //   bits  0-2:  faceIndex    (0-5)
    //   bits  3-10: textureIndex (0-255)
    //   bits 11-14: lightLevel   (0-15)
    //   bits 15-16: uvRotation   (0-3) - currently unused in output, available for future use
    uint faceIndex    = input.packed & 0x7;
    uint textureIndex = (input.packed >> 3) & 0xFF;
    uint lightLevel   = (input.packed >> 11) & 0xF;
    // uint uvRotation = (input.packed >> 15) & 0x3;

    // Transform position to world space
    float4 worldPos = mul(float4(input.position, 1.0), world);
    output.worldPosition = worldPos.xyz;

    // Transform to clip space
    output.position = mul(worldPos, viewProjection);

    // Look up normal from face index
    output.normal = faceNormals[min(faceIndex, 5)];

    // Pass through data
    output.texCoord = input.texCoord;
    output.lightLevel = (float)lightLevel;
    output.textureIndex = textureIndex;
    output.fogFactor = 1.0;

    return output;
}
