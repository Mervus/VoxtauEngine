Texture2D entityTexture : register(t0);
SamplerState texSampler : register(s0);

struct PixelInput {
    float4 position      : SV_POSITION;
    float3 worldPosition : WORLD_POS;
    float3 normal        : NORMAL;
    float2 texCoord      : TEXCOORD0;
    float4 color         : COLOR;
};

float4 main(PixelInput input) : SV_TARGET {
    // Start with vertex color
    float4 baseColor = input.color;

    // Multiply by texture if one is bound (non-zero alpha)
    float4 texColor = entityTexture.Sample(texSampler, input.texCoord);
    if (texColor.a > 0.01)
        baseColor *= texColor;

    // Directional lighting
    float3 lightDir = normalize(float3(0.3, 0.7, 0.5));
    float ambient = 0.3;
    float diffuse = max(dot(normalize(input.normal), lightDir), 0.0) * 0.7;
    float lighting = ambient + diffuse;

    float3 finalColor = baseColor.rgb * lighting;

    // Alpha discard
    if (baseColor.a < 0.1)
        discard;

    return float4(finalColor, baseColor.a);
}