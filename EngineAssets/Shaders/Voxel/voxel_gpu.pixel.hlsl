// voxel_gpu.pixel.hlsl — Pixel shader for GPU-meshed chunks
// Receives per-vertex AO interpolated across the face

// Frame constants (slot 0)
cbuffer PerFrameBuffer : register(b0)
{
    float4x4 ViewProjection;
    float3   CameraPosition;
    float    _pad0;
};

// Block texture array
Texture2DArray BlockTextures : register(t0);
SamplerState TextureSampler : register(s0);

// Input from vertex shader
struct PSInput
{
    float4 position  : SV_POSITION;
    float3 worldPos  : WORLDPOS;
    float3 normal    : NORMAL;
    float2 uv        : TEXCOORD0;
    uint   blockType : BLOCKTYPE;
    float  ao        : AMBIENT_OCCLUSION;
};

float4 main(PSInput input) : SV_TARGET
{
    // Sample texture from array using blockType as index
    float3 texCoord = float3(input.uv, float(input.blockType));
    float4 texColor = BlockTextures.Sample(TextureSampler, texCoord);

    // Simple directional lighting
    float3 lightDir = normalize(float3(0.3, 0.7, 0.5));
    float3 normal = normalize(input.normal);

    // Ambient + diffuse
    float ambient = 0.3;
    float diffuse = max(dot(normal, lightDir), 0.0) * 0.7;
    float lighting = ambient + diffuse;

    // Apply lighting and AO
    // AO is interpolated across the quad — the hardware interpolates the float
    // between corners, giving smooth gradients across the face
    float3 finalColor = texColor.rgb * lighting * input.ao;

    // Alpha test for transparent blocks
    if (texColor.a < 0.1)
        discard;

    return float4(finalColor, texColor.a);
}