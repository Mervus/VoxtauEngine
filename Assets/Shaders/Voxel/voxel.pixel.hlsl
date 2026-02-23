// VOXEL PIXEL SHADER - Simple textured output

// TEXTURES & SAMPLERS
Texture2DArray blockTextures : register(t0);
SamplerState textureSampler  : register(s0);

// INPUT (matches vertex shader output)
struct PixelInput
{
    float4 position      : SV_POSITION;
    float3 worldPosition : WORLD_POS;
    float3 normal        : NORMAL;
    float2 texCoord      : TEXCOORD0;
    float  lightLevel    : LIGHT;
    uint   textureIndex  : TEXINDEX;
    float  fogFactor     : FOG;
};

// PIXEL SHADER MAIN
float4 main(PixelInput input) : SV_TARGET
{
    // Sample texture from array
    float3 texCoord3D = float3(input.texCoord, (float)input.textureIndex);
    float4 texColor = blockTextures.Sample(textureSampler, texCoord3D);

    // Alpha test (for leaves, etc.)
    if (texColor.a < 0.5)
        discard;

    return texColor;
}
