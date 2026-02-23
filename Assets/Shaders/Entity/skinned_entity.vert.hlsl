// Vertex shader for skeletal animation
//
// Same PerFrameBuffer (b0) and PerObjectBuffer (b1) as entity.vert.hlsl
// Added BoneBuffer (b2) with the bone matrix palette from the Animator

cbuffer PerFrameBuffer : register(b0) {
    matrix viewProjection;
    float3 cameraPosition;
    float  padding0;
};

cbuffer PerObjectBuffer : register(b1) {
    matrix world;
    matrix worldInverseTranspose;
};

// Bone matrices — MAX_BONES = 128
// Each matrix is: inverseBindPose * currentWorldPose
cbuffer BoneBuffer : register(b2) {
    matrix boneMatrices[128];
};

struct VertexInput {
    float3 position    : POSITION;
    float3 normal      : NORMAL;
    float2 texCoord    : TEXCOORD;
    float4 color       : COLOR;
    float3 tangent     : TANGENT;
    uint4  boneIndices : BONEINDICES;
    float4 boneWeights : BONEWEIGHTS;
};

struct VertexOutput {
    float4 position      : SV_POSITION;
    float3 worldPosition : WORLD_POS;
    float3 normal        : NORMAL;
    float2 texCoord      : TEXCOORD0;
    float4 color         : COLOR;
};

VertexOutput main(VertexInput input) {
    VertexOutput output;

    // Compute skinned position and normal by blending bone transforms
    float4 skinnedPos = float4(0, 0, 0, 0);
    float3 skinnedNorm = float3(0, 0, 0);

    float totalWeight = input.boneWeights.x + input.boneWeights.y +
                        input.boneWeights.z + input.boneWeights.w;

    if (totalWeight > 0.0) {
        // Apply bone influences
        for (int i = 0; i < 4; i++) {
            float w = 0;
            uint  idx = 0;

            // Unroll manually since HLSL indexing into float4 needs this
            if (i == 0)      { w = input.boneWeights.x; idx = input.boneIndices.x; }
            else if (i == 1) { w = input.boneWeights.y; idx = input.boneIndices.y; }
            else if (i == 2) { w = input.boneWeights.z; idx = input.boneIndices.z; }
            else             { w = input.boneWeights.w; idx = input.boneIndices.w; }

            if (w > 0.0) {
                // Row-vector convention: vertex * boneMatrix
                skinnedPos  += mul(float4(input.position, 1.0), boneMatrices[idx]) * w;
                skinnedNorm += mul(float4(input.normal, 0.0),   boneMatrices[idx]).xyz * w;
            }
        }
    } else {
        // No bone weights — pass through unskinned
        skinnedPos = float4(input.position, 1.0);
        skinnedNorm = input.normal;
    }

    // Transform skinned position to world space
    float4 worldPos = mul(skinnedPos, world);
    output.position = mul(worldPos, viewProjection);
    output.worldPosition = worldPos.xyz;

    // Transform skinned normal to world space
    output.normal = normalize(mul(float4(skinnedNorm, 0.0), worldInverseTranspose).xyz);

    output.texCoord = input.texCoord;
    output.color = input.color;

    return output;
}
