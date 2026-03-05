// TEXTURES & SAMPLERS
Texture2D uiTexture : register(t0);
SamplerState uiSampler : register(s0);

// CONSTANT BUFFERS
cbuffer UIElementBuffer : register(b1)
{
    float2 position;
    float2 size;
    float2 pivot;
    float rotation;
    float depth;
    float4 tintColor;
    float2 uvOffset;
    float2 uvScale;
};

cbuffer UIEffectsBuffer : register(b2)
{
    float cornerRadius;      // Rounded corners (pixels)
    float borderWidth;       // Border thickness (pixels)
    float4 borderColor;      // Border color
    float shadowOffset;      // Shadow offset (pixels)
    float shadowBlur;        // Shadow blur radius
    float4 shadowColor;      // Shadow color
    float opacity;           // Overall opacity (0-1)
    float3 padding1;
};

// INPUT (from vertex shader)
struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float4 color    : COLOR;
    float2 localPos : LOCAL_POS;
};

// HELPER FUNCTIONS

// Rounded rectangle SDF (signed distance field)
float RoundedRectSDF(float2 pos, float2 size, float radius)
{
    float2 d = abs(pos - 0.5) * size - (size * 0.5 - radius);
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - radius;
}

// PIXEL SHADER MAIN
float4 main(PixelInput input) : SV_TARGET
{
    // 1. SAMPLE TEXTURE
    float4 texColor = uiTexture.Sample(uiSampler, input.texCoord);

    // 2. APPLY VERTEX COLOR
    float4 finalColor = texColor * input.color;

    // 3. ROUNDED CORNERS (if enabled)
    if (cornerRadius > 0.0)
    {
        float dist = RoundedRectSDF(input.localPos, float2(1, 1), cornerRadius / size.x);

        // Anti-aliased edge
        float alpha = 1.0 - smoothstep(-1.0, 1.0, dist);
        finalColor.a *= alpha;

        // Discard fully transparent pixels
        if (finalColor.a < 0.01)
        {
            discard;
        }
    }

    // 4. BORDER (if enabled)
    if (borderWidth > 0.0)
    {
        float dist = RoundedRectSDF(input.localPos, float2(1, 1), cornerRadius / size.x);
        float borderDist = abs(dist) - borderWidth / size.x;

        if (borderDist < 0.0)
        {
            // We're in the border region
            float borderAlpha = 1.0 - smoothstep(-1.0, 0.0, borderDist);
            finalColor = lerp(finalColor, borderColor, borderAlpha * borderColor.a);
        }
    }

    // 5. APPLY OVERALL OPACITY
    finalColor.a *= opacity;

    return finalColor;
}