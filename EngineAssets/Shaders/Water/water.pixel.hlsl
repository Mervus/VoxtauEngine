// CONSTANT BUFFERS
cbuffer PerFrameBuffer : register(b0)
{
    matrix viewProjection;
    float3 cameraPosition;
    float time;
    float3 sunDirection;
    float padding0;
    float3 sunColor;
    float sunIntensity;
    float3 ambientColor;
    float ambientIntensity;
};

cbuffer WaterPropertiesBuffer : register(b2)
{
    float3 waterColor;        // Base water color (blue-green)
    float waterAlpha;         // Base transparency
    float3 shallowColor;      // Color in shallow water
    float shallowDepth;       // Depth at which shallow color fades
    float3 deepColor;         // Color in deep water
    float refractionStrength; // Distortion amount
    float reflectionStrength; // How much reflection
    float fresnelPower;       // Fresnel effect strength
    float foamAmount;         // Foam intensity
    float foamDepth;          // Depth at which foam appears
};

// TEXTURES & SAMPLERS
Texture2D normalMap       : register(t0); // Water normal map
Texture2D sceneTexture    : register(t1); // Scene behind water (for refraction)
Texture2D depthTexture    : register(t2); // Depth buffer
Texture2D reflectionMap   : register(t3); // Reflection texture (optional)
Texture2D foamTexture     : register(t4); // Foam pattern

SamplerState linearSampler  : register(s0);
SamplerState pointSampler   : register(s1);

// INPUT
struct PixelInput
{
    float4 position       : SV_POSITION;
    float3 worldPosition  : WORLD_POS;
    float3 normal         : NORMAL;
    float2 texCoord       : TEXCOORD0;
    float4 screenPosition : TEXCOORD1;
    float3 viewDirection  : TEXCOORD2;
    float3 tangent        : TANGENT;
    float3 bitangent      : BITANGENT;
};

// HELPER FUNCTIONS

// Calculate Fresnel effect (edges more reflective)
float FresnelSchlick(float3 viewDir, float3 normal, float power)
{
    float cosTheta = saturate(dot(viewDir, normal));
    return pow(1.0 - cosTheta, power);
}

// Sample normal map and convert to world space
float3 SampleNormalMap(float2 uv, float3 normal, float3 tangent, float3 bitangent)
{
    // Sample normal map (tangent space)
    float3 normalTS = normalMap.Sample(linearSampler, uv).xyz;
    normalTS = normalize(normalTS * 2.0 - 1.0);

    // Transform to world space
    float3x3 TBN = float3x3(tangent, bitangent, normal);
    return normalize(mul(normalTS, TBN));
}

// Get screen-space UV coordinates
float2 GetScreenUV(float4 screenPos)
{
    float2 uv = screenPos.xy / screenPos.w;
    uv = uv * 0.5 + 0.5;
    uv.y = 1.0 - uv.y; // Flip Y
    return uv;
}

// Calculate water depth from depth buffer
float GetWaterDepth(float2 screenUV, float waterDepth)
{
    float sceneDepth = depthTexture.Sample(pointSampler, screenUV).r;
    // Convert depth buffer value to linear depth (this depends on your projection matrix)
    // Simplified version:
    return sceneDepth - waterDepth;
}

// PIXEL SHADER MAIN
float4 main(PixelInput input) : SV_TARGET
{
    // 1. SAMPLE NORMAL MAP (for detail)
    float3 detailNormal1 = SampleNormalMap(input.texCoord * 2.0, input.normal, input.tangent, input.bitangent);
    float3 detailNormal2 = SampleNormalMap(input.texCoord * 3.0 - float2(time * 0.01, 0), input.normal, input.tangent, input.bitangent);

    // Combine normals
    float3 finalNormal = normalize(input.normal + detailNormal1 * 0.3 + detailNormal2 * 0.2);

    // 2. SCREEN-SPACE COORDINATES
    float2 screenUV = GetScreenUV(input.screenPosition);

    // 3. REFRACTION (distort background)
    float2 refractOffset = finalNormal.xz * refractionStrength;
    float2 refractUV = screenUV + refractOffset;
    refractUV = saturate(refractUV); // Clamp to screen bounds

    float3 refractionColor = sceneTexture.Sample(linearSampler, refractUV).rgb;

    // 4. REFLECTION
    float2 reflectUV = screenUV + finalNormal.xz * 0.05;
    reflectUV.y = 1.0 - reflectUV.y; // Flip for reflection

    float3 reflectionColor = reflectionMap.Sample(linearSampler, reflectUV).rgb;

    // 5. FRESNEL EFFECT
    float fresnel = FresnelSchlick(input.viewDirection, finalNormal, fresnelPower);

    // 6. WATER DEPTH & COLOR
    float waterDepth = GetWaterDepth(screenUV, input.screenPosition.w);

    // Blend between shallow and deep color based on depth
    float depthFactor = saturate(waterDepth / shallowDepth);
    float3 depthColor = lerp(shallowColor, deepColor, depthFactor);

    // 7. FOAM (at edges/shallow areas)
    float foamFactor = 1.0 - saturate(waterDepth / foamDepth);
    float foamPattern = foamTexture.Sample(linearSampler, input.texCoord * 5.0).r;
    float foam = foamFactor * foamPattern * foamAmount;

    // 8. SPECULAR HIGHLIGHTS (sun reflection)
    float3 reflectDir = reflect(-sunDirection, finalNormal);
    float spec = pow(max(dot(reflectDir, input.viewDirection), 0.0), 64.0);
    float3 specular = sunColor * spec * sunIntensity * 0.5;

    // 9. COMBINE ALL EFFECTS

    // Base water color
    float3 baseColor = waterColor * depthColor;

    // Mix refraction with base color
    float3 finalColor = lerp(refractionColor, baseColor, waterAlpha);

    // Add reflection based on fresnel
    finalColor = lerp(finalColor, reflectionColor, fresnel * reflectionStrength);

    // Add foam
    finalColor = lerp(finalColor, float3(1, 1, 1), foam);

    // Add specular highlights
    finalColor += specular;

    // 10. CALCULATE FINAL ALPHA
    float finalAlpha = lerp(waterAlpha, 1.0, foam); // Foam is opaque
    finalAlpha = lerp(finalAlpha, 0.9, fresnel * 0.5); // Edges more opaque

    return float4(finalColor, finalAlpha);
}