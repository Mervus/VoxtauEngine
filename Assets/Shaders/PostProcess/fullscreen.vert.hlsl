// Fullscreen Quad Vertex Shader
// Used for post-processing effects

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
    float2 texCoord : TEXCOORD0;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;

    // Pass through position directly (already in NDC space: -1 to 1)
    output.position = float4(input.position.xy, 0.0, 1.0);

    // Pass through texture coordinates
    output.texCoord = input.texCoord;

    return output;
}
