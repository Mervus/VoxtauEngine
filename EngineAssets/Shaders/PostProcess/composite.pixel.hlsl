// composite.pixel.hlsl — Blends a low-res effect result over full-res scene
//
// t0 = full-res scene color
// t2 = low-res effect result (upscaled via linear sampler)
//
// The effect shader outputs alpha to control blending:
//   alpha = 0: scene shows through (no effect)
//   alpha = 1: fully replaced by effect color

Texture2D sceneColor  : register(t0);
Texture2D effectColor : register(t2);

SamplerState pointSampler  : register(s0);
SamplerState linearSampler : register(s2);

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float4 main(PixelInput input) : SV_TARGET
{
    float4 scene  = sceneColor.Sample(pointSampler, input.texCoord);
    float4 effect = effectColor.Sample(linearSampler, input.texCoord);

    // Alpha blend: effect over scene
    float3 result = lerp(scene.rgb, effect.rgb, effect.a);

    return float4(result, 1.0);
}
