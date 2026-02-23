cbuffer PerFrameBuffer : register(b0) {
    matrix viewProjection;
    float3 cameraPosition;
    float padding0;
};

cbuffer PerObjectBuffer : register(b1) {
    matrix world;
    matrix worldInverseTranspose;
};

struct VertexInput {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
    float4 color    : COLOR;
    float3 tangent  : TANGENT;
    float  lightLevel : LIGHT;
    uint   textureIndex : TEXINDEX;
    uint   uvRotation : UVROTATION;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

VertexOutput main(VertexInput input) {
    VertexOutput output;
    float4 worldPos = mul(float4(input.position, 1.0), world);
    output.position = mul(worldPos, viewProjection);
    output.color = input.color;
    return output;
}
