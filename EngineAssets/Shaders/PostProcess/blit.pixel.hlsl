// blit.pixel.hlsl — Simple copy from scene render target to backbuffer
// Can be extended with tone mapping later

Texture2D sceneColor : register(t0);
SamplerState linearSampler : register(s0);

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float4 main(PixelInput input) : SV_TARGET
{
    float4 color = sceneColor.Sample(linearSampler, input.texCoord);

    // Simple Reinhard tone mapping (optional — remove for raw HDR passthrough)
    // color.rgb = color.rgb / (color.rgb + 1.0);

    // Gamma correction (if rendering in linear space)
    // color.rgb = pow(color.rgb, 1.0 / 2.2);

    return color;
}
