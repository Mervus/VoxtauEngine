// Tone Mapping Pixel Shader
// Converts HDR colors to displayable LDR output

// Post-processing constant buffer
cbuffer PostProcessBuffer : register(b0)
{
    float exposure;         // HDR exposure adjustment
    float gamma;            // Gamma correction (usually 2.2)
    int tonemapOperator;    // 0=ACES, 1=Reinhard, 2=Uncharted2
    float padding;
};

// HDR scene texture
Texture2D sceneTexture : register(t0);
SamplerState sceneSampler : register(s0);

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

// ACES Filmic Tone Mapping
// Industry standard, good color preservation
float3 ACESFilmic(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// Reinhard Tone Mapping
// Simple, prevents clipping, can look washed out
float3 Reinhard(float3 x)
{
    return x / (1.0f + x);
}

// Extended Reinhard with white point
float3 ReinhardExtended(float3 x, float whitePoint)
{
    float3 numerator = x * (1.0f + (x / (whitePoint * whitePoint)));
    return numerator / (1.0f + x);
}

// Uncharted 2 Filmic Tone Mapping
// Good for games, nice filmic look
float3 Uncharted2Partial(float3 x)
{
    float A = 0.15f; // Shoulder Strength
    float B = 0.50f; // Linear Strength
    float C = 0.10f; // Linear Angle
    float D = 0.20f; // Toe Strength
    float E = 0.02f; // Toe Numerator
    float F = 0.30f; // Toe Denominator

    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 Uncharted2(float3 color)
{
    float exposureBias = 2.0f;
    float3 curr = Uncharted2Partial(color * exposureBias);

    float3 W = float3(11.2f, 11.2f, 11.2f); // White point
    float3 whiteScale = 1.0f / Uncharted2Partial(W);

    return curr * whiteScale;
}

float4 main(PixelInput input) : SV_TARGET
{
    // Sample HDR scene
    float4 sampled = sceneTexture.Sample(sceneSampler, input.texCoord);
    float3 hdrColor = sampled.rgb;

    // Apply exposure and ensure non-negative values
    hdrColor = max(0.0f, hdrColor * exposure);

    // Passthrough mode - skip all processing
    if (tonemapOperator == 3)
    {
        return float4(hdrColor, 1.0f);
    }

    // Apply tone mapping based on selected operator
    // 0=ACES, 1=Reinhard, 2=Uncharted2
    float3 ldrColor;
    if (tonemapOperator == 0)
    {
        ldrColor = ACESFilmic(hdrColor);
    }
    else if (tonemapOperator == 1)
    {
        ldrColor = Reinhard(hdrColor);
    }
    else // tonemapOperator == 2
    {
        ldrColor = Uncharted2(hdrColor);
    }

    // Apply gamma correction (use abs to avoid pow issues with negative values)
    ldrColor = pow(abs(ldrColor), 1.0f / gamma);

    return float4(ldrColor, 1.0f);
}
