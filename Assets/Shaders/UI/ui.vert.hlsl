// Assets/Shaders/UI/ui.vert.hlsl

// ==========================================
// CONSTANT BUFFERS
// ==========================================
cbuffer UITransformBuffer : register(b0)
{
    matrix projection;  // Orthographic projection (screen space)
    float2 screenSize;  // Screen dimensions (width, height)
    float2 padding0;
};

cbuffer UIElementBuffer : register(b1)
{
    float2 position;    // Element position (pixels)
    float2 size;        // Element size (pixels)
    float2 pivot;       // Pivot point (0-1, for rotation)
    float rotation;     // Rotation in radians
    float depth;        // Z-order (0-1, closer to 0 = in front)
    float4 tintColor;   // Color tint/multiply
    float2 uvOffset;    // UV offset (for texture scrolling)
    float2 uvScale;     // UV scale (for texture tiling)
};

// ==========================================
// INPUT (from vertex buffer)
// ==========================================
struct VertexInput
{
    float2 position : POSITION;  // Local position (0-1)
    float2 texCoord : TEXCOORD;  // UV coordinates (0-1)
    float4 color    : COLOR;     // Vertex color
};

// ==========================================
// OUTPUT (to pixel shader)
// ==========================================
struct VertexOutput
{
    float4 position : SV_POSITION;  // Screen position
    float2 texCoord : TEXCOORD;
    float4 color    : COLOR;
    float2 localPos : LOCAL_POS;    // Position within element (0-1)
};

// HELPER FUNCTIONS

// 2D rotation matrix
float2x2 Rotate2D(float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return float2x2(c, -s, s, c);
}

// VERTEX SHADER MAIN
VertexOutput main(VertexInput input)
{
    VertexOutput output;

    // 1. LOCAL SPACE (0-1 quad)
    float2 localPos = input.position; // 0-1 range
    output.localPos = localPos;

    // 2. APPLY PIVOT & ROTATION
    float2 centered = localPos - pivot;

    if (rotation != 0.0)
    {
        centered = mul(centered, Rotate2D(rotation));
    }

    float2 rotated = centered + pivot;

    // 3. SCALE TO PIXEL SIZE
    float2 scaled = rotated * size;

    // 4. TRANSLATE TO SCREEN POSITION
    float2 screenPos = scaled + position;

    // 5. CONVERT TO CLIP SPACE (-1 to 1)
    // DirectX: (0,0) top-left, Y down
    float2 clipPos;
    clipPos.x = (screenPos.x / screenSize.x) * 2.0 - 1.0;
    clipPos.y = 1.0 - (screenPos.y / screenSize.y) * 2.0;