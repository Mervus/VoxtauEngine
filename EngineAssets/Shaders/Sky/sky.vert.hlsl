// CONSTANT BUFFERS
cbuffer SkyBuffer : register(b0)
{
    matrix viewProjection;
    float3 cameraPosition;
    float time;
};

// INPUT (skybox cube or dome)
struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
};

// OUTPUT
struct VertexOutput
{
    float4 position      : SV_POSITION;
    float3 worldPosition : WORLD_POS;   // Used for atmospheric calculations
    float3 viewDirection : VIEW_DIR;    // Direction from camera
    float2 texCoord      : TEXCOORD;
};

// VERTEX SHADER MAIN
VertexOutput main(VertexInput input)
{
    VertexOutput output;

    // Skybox is always centered on camera (no translation)
    float3 worldPos = input.position + cameraPosition;
    output.worldPosition = worldPos;

    // Transform to clip space
    // Remove translation from view matrix (skybox stays at infinity)
    float4 clipPos = mul(float4(input.position, 0.0), viewProjection);
    output.position = clipPos.xyww; // Set z = w (ensures z/w = 1.0, far plane)

    // View direction (from camera through this vertex)
    output.viewDirection = normalize(input.position);

    // Pass through texture coordinates
    output.texCoord = input.texCoord;

    return output;
}