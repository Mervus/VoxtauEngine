// distant_terrain.pixel.hlsl
// Flat-shaded for blocky voxel look

struct PixelInput {
    float4 position      : SV_POSITION;
    float3 worldPosition : WORLD_POS;
    float3 normal        : NORMAL;
    float4 color         : COLOR;
    float  fogFactor     : FOG;
};

float4 main(PixelInput input) : SV_TARGET {
    // Recompute flat normal from screen-space derivatives
    // This gives hard per-triangle shading — no smooth interpolation
    float3 dpdx = ddx(input.worldPosition);
    float3 dpdy = ddy(input.worldPosition);
    float3 flatNormal = normalize(cross(dpdx, dpdy));

    // Sun
    float3 lightDir = normalize(float3(0.3, 0.7, 0.5));
    float NdotL = max(dot(flatNormal, lightDir), 0.0);

    // Fill
    float3 fillDir = normalize(float3(-0.4, 0.3, -0.3));
    float fillLight = max(dot(flatNormal, fillDir), 0.0) * 0.15;

    float ambient = 0.35;
    float lighting = ambient + NdotL * 0.6 + fillLight;

    // Slope-based rock blending using flat normal
    float slope = 1.0 - flatNormal.y;
    float3 baseColor = input.color.rgb;
    float3 rockColor = float3(0.45, 0.40, 0.35);
    baseColor = lerp(baseColor, rockColor, saturate(slope * 2.0 - 0.4));

    float3 litColor = baseColor * lighting;

    // Light fog at extreme distance
    float3 fogColor = float3(0.65, 0.72, 0.85);
    float3 finalColor = lerp(fogColor, litColor, input.fogFactor);

    return float4(finalColor, 1.0);
}
