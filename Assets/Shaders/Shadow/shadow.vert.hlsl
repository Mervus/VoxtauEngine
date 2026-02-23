// Shadow Map Vertex Shader
// Transforms vertices to light space for depth rendering

cbuffer PerObjectBuffer : register(b1)
{
    matrix world;
    matrix worldInverseTranspose;
};

cbuffer ShadowBuffer : register(b2)
{
    matrix lightViewProjection;
    float shadowBias;
    float shadowMapSize;
    float shadowStrength;
    float padding;
};

struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
    float4 color    : COLOR;
    float3 tangent  : TANGENT;
    float  light    : LIGHT;
    uint   texIndex : TEXINDEX;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;

    // Transform to world space
    float4 worldPos = mul(float4(input.position, 1.0), world);

    // Transform to light clip space
    output.position = mul(worldPos, lightViewProjection);

    // Pass through tex coord for alpha testing
    output.texCoord = input.texCoord;

    return output;
}
