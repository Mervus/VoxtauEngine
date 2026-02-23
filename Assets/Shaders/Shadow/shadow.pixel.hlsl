// Shadow Map Pixel Shader
// For opaque objects, this shader does nothing (depth is written automatically)
// For alpha-tested objects, we discard transparent pixels

Texture2DArray textureArray : register(t0);
SamplerState textureSampler : register(s0);

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

void main(PixelInput input)
{
    // For now, just write depth (no alpha testing)
    // When we need alpha testing for leaves, etc., we can sample the texture:
    // float4 texColor = textureArray.Sample(textureSampler, float3(input.texCoord, 0));
    // if (texColor.a < 0.5) discard;
}
