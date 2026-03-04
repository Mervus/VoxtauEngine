 //EFFECT SHADER TEMPLATE
 Texture2D sceneColor : register(t0);
 Texture2D sceneDepth : register(t1);
 SamplerState linearSampler : register(s0);

 cbuffer PostProcessConstants : register(b1)
 {
     float screenWidth;
     float screenHeight;
     float time;
     float padding0;
     float3 cameraPosition;
     float padding1;
     float4x4 inverseViewProjection;
 };

 struct PixelInput
 {
     float4 position : SV_POSITION;
     float2 texCoord : TEXCOORD0;
 };

 float3 ReconstructWorldPos(float2 uv, float depth)
 {
     float2 ndc = uv * 2.0 - 1.0;
     ndc.y = -ndc.y;
     float4 clip = float4(ndc, depth, 1.0);
     float4 world = mul(clip, inverseViewProjection);
     return world.xyz / world.w;
 }

 float4 main(PixelInput input) : SV_TARGET
 {
     float4 color = sceneColor.Sample(linearSampler, input.texCoord);
     float depth = sceneDepth.Sample(linearSampler, input.texCoord).r;
     float3 worldPos = ReconstructWorldPos(input.texCoord, depth);

     // effect here...
     // Return float4(effectColor, alpha) for scaled effects

     return color;
 }