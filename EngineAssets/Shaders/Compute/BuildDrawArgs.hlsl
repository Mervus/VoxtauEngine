// BuildDrawArgs.hlsl
RWByteAddressBuffer FaceCounter : register(u0);
RWStructuredBuffer<uint> DrawArgs : register(u1);

[numthreads(1, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID) {
    uint faceCount = FaceCounter.Load(0);
    uint indexCount = faceCount * 6;

    // DrawIndexedInstancedIndirect args layout:
    // [0] IndexCountPerInstance
    // [1] InstanceCount
    // [2] StartIndexLocation
    // [3] BaseVertexLocation
    // [4] StartInstanceLocation
    DrawArgs[0] = indexCount;
    DrawArgs[1] = 1;
    DrawArgs[2] = 0;
    DrawArgs[3] = 0;
    DrawArgs[4] = 0;
}
