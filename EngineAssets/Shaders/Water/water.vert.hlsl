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

cbuffer PerObjectBuffer : register(b1)
{
    matrix world;
    matrix worldInverseTranspose;
};

cbuffer WaterBuffer : register(b2)
{
    float waveSpeed;
    float waveHeight;
    float waveFrequency;
    float padding1;
    float2 waveDirection1;
    float2 waveDirection2;
};

// INPUT
struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
    float4 color    : COLOR;
    float3 tangent  : TANGENT;
    float  light    : LIGHT;
};

// OUTPUT
struct VertexOutput
{
    float4 position       : SV_POSITION;  // Clip space position
    float3 worldPosition  : WORLD_POS;    // World space position
    float3 normal         : NORMAL;       // World space normal
    float2 texCoord       : TEXCOORD0;
    float4 screenPosition : TEXCOORD1;    // For screen-space effects
    float3 viewDirection  : TEXCOORD2;    // Camera to vertex
    float3 tangent        : TANGENT;
    float3 bitangent      : BITANGENT;
};

// WAVE FUNCTIONS

// Gerstner wave (realistic ocean waves)
float3 GerstnerWave(float3 position, float2 direction, float frequency, float amplitude, float speed, float time)
{
    float2 d = normalize(direction);
    float k = 2.0 * 3.14159 / frequency; // Wave number
    float c = sqrt(9.8 / k); // Wave speed (based on physics)
    float2 dir = d * k;

    float f = k * (dot(d, position.xz) - c * speed * time);
    float a = amplitude / k;

    float3 wave;
    wave.x = d.x * a * cos(f);
    wave.y = a * sin(f);
    wave.z = d.y * a * cos(f);

    return wave;
}

// Simple sine wave (cheaper, less realistic)
float SimpleWave(float3 position, float2 direction, float time)
{
    float wave = sin(dot(position.xz, direction) * waveFrequency + time * waveSpeed);
    return wave * waveHeight;
}

// VERTEX SHADER MAIN
VertexOutput main(VertexInput input)
{
    VertexOutput output;

    // Start with base position
    float3 worldPos = mul(float4(input.position, 1.0), world).xyz;

    // ==========================================
    // ANIMATED WAVES (vertex displacement)
    // ==========================================

    // Wave 1
    float wave1 = SimpleWave(worldPos, waveDirection1, time);

    // Wave 2 (different direction for more complex motion)
    float wave2 = SimpleWave(worldPos, waveDirection2, time * 0.8);

    // Combine waves
    worldPos.y += wave1 + wave2 * 0.5;

    // Alternative: Use Gerstner waves for more realism (more expensive)
    // float3 gerstner1 = GerstnerWave(worldPos, waveDirection1, 10.0, 0.2, 1.0, time);
    // float3 gerstner2 = GerstnerWave(worldPos, waveDirection2, 8.0, 0.15, 1.2, time * 0.9);
    // worldPos += gerstner1 + gerstner2 * 0.5;

    output.worldPosition = worldPos;

    // TRANSFORM TO CLIP SPACE
    output.position = mul(float4(worldPos, 1.0), viewProjection);

    // CALCULATE DYNAMIC NORMAL (from wave displacement)

    // Sample nearby positions to calculate gradient
    float offset = 0.1;

    float heightL = SimpleWave(worldPos - float3(offset, 0, 0), waveDirection1, time)
                  + SimpleWave(worldPos - float3(offset, 0, 0), waveDirection2, time * 0.8) * 0.5;
    float heightR = SimpleWave(worldPos + float3(offset, 0, 0), waveDirection1, time)
                  + SimpleWave(worldPos + float3(offset, 0, 0), waveDirection2, time * 0.8) * 0.5;
    float heightD = SimpleWave(worldPos - float3(0, 0, offset), waveDirection1, time)
                  + SimpleWave(worldPos - float3(0, 0, offset), waveDirection2, time * 0.8) * 0.5;
    float heightU = SimpleWave(worldPos + float3(0, 0, offset), waveDirection1, time)
                  + SimpleWave(worldPos + float3(0, 0, offset), waveDirection2, time * 0.8) * 0.5;

    float3 tangentX = normalize(float3(2.0 * offset, heightR - heightL, 0));
    float3 tangentZ = normalize(float3(0, heightU - heightD, 2.0 * offset));
    output.normal = normalize(cross(tangentZ, tangentX));

    // SCREEN POSITION (for screen-space effects)
    output.screenPosition = output.position;

    // VIEW DIRECTION (for fresnel)
    output.viewDirection = normalize(cameraPosition - worldPos);

    // TANGENT SPACE (for normal mapping)
    output.tangent = normalize(mul(input.tangent, (float3x3)world));
    output.bitangent = cross(output.normal, output.tangent);

    // TEXTURE COORDINATES (animate for flow)
    output.texCoord = input.texCoord + float2(time * 0.02, time * 0.03);

    return output;
}