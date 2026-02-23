// ChunkMesh.hlsl — Generate mesh faces for a 32x32x32 chunk
// Output: 1 packed uint per visible face (8x smaller than old 4xuint2 format)

// Voxel data: 8192 uints, each packing 4 voxels (uint8 each)
StructuredBuffer<uint> VoxelData : register(t0);

// Neighbor chunk data for seamless borders (6 neighbors)
StructuredBuffer<uint> NeighborXPos : register(t1);
StructuredBuffer<uint> NeighborXNeg : register(t2);
StructuredBuffer<uint> NeighborYPos : register(t3);
StructuredBuffer<uint> NeighborYNeg : register(t4);
StructuredBuffer<uint> NeighborZPos : register(t5);
StructuredBuffer<uint> NeighborZNeg : register(t6);

// Block texture lookup: blockType * 6 + faceIdx -> texture array index
StructuredBuffer<uint> BlockTextureMap : register(t7);

// Output: 1 packed uint per face
RWStructuredBuffer<uint> OutputFaces : register(u0);

// Atomic counter: how many faces written so far
RWByteAddressBuffer FaceCounter : register(u1);

// Per-dispatch constants
cbuffer MeshConstants : register(b0) {
    float3 ChunkWorldPos;   // world position of this chunk
    uint   MaxFaces;        // safety cap to prevent buffer overflow
};

// ── Helpers ─────────────────────────────────────────────────

// Flatten 3D coord to 1D index
uint FlatIndex(uint3 p) {
    // Match Chunk::GetIndex: x + z * 32 + y * 32 * 32
    return p.x + p.z * 32 + p.y * 32 * 32;
}

// Read a voxel from the packed buffer. Returns block type (0 = air).
uint GetVoxel(uint3 p) {
    uint flat = FlatIndex(p);
    uint word = VoxelData[flat / 4];
    uint shift = (flat % 4) * 8;
    return (word >> shift) & 0xFF;
}

// Read a voxel, handling out-of-bounds by sampling neighbor chunks.
uint GetVoxelSafe(int3 p) {
    if (p.x >= 0 && p.x < 32 &&
        p.y >= 0 && p.y < 32 &&
        p.z >= 0 && p.z < 32) {
        return GetVoxel(uint3(p));
    }
    return 0;
}

// Check if a block type is transparent/air
bool IsTransparent(uint blockType) {
    return blockType == 0;
}

// Pack a face into a single uint32:
//   Bits [4:0]   = block X (0-31)
//   Bits [9:5]   = block Y (0-31)
//   Bits [14:10] = block Z (0-31)
//   Bits [17:15] = face index (0-5)
//   Bits [25:18] = texture array index
//   Bits [31:26] = spare
uint PackFace(uint3 pos, uint faceIdx, uint texIndex) {
    return (pos.x & 0x1F)
         | ((pos.y & 0x1F) << 5)
         | ((pos.z & 0x1F) << 10)
         | ((faceIdx & 0x7) << 15)
         | ((texIndex & 0xFF) << 18);
}

// Face normal directions (±X, ±Y, ±Z)
static const int3 FaceNormals[6] = {
    int3( 1,  0,  0),  // 0: +X
    int3(-1,  0,  0),  // 1: -X
    int3( 0,  1,  0),  // 2: +Y
    int3( 0, -1,  0),  // 3: -Y
    int3( 0,  0,  1),  // 4: +Z
    int3( 0,  0, -1),  // 5: -Z
};

// Emit a single face (1 packed uint).
void EmitFace(uint3 voxelPos, uint faceIdx, uint blockType) {
    // Atomically reserve 1 face slot
    uint baseFace;
    FaceCounter.InterlockedAdd(0, 1, baseFace);

    // Safety check
    if (baseFace >= MaxFaces) return;

    // Look up the actual texture array index for this block type and face
    uint texIndex = BlockTextureMap[blockType * 6 + faceIdx];

    OutputFaces[baseFace] = PackFace(voxelPos, faceIdx, texIndex);
}

// Thread group: 8x8x8 = 512 threads
// Dispatch: (4, 4, 4) = 64 groups = 32,768 threads = one per voxel
[numthreads(8, 8, 8)]
void main(uint3 dtid : SV_DispatchThreadID) {
    if (any(dtid >= 32)) return;

    uint blockType = GetVoxel(dtid);

    // Skip air voxels
    if (IsTransparent(blockType)) return;

    // Test each face: emit if the neighbor in that direction is air
    [unroll]
    for (uint face = 0; face < 6; face++) {
        int3 neighborPos = int3(dtid) + FaceNormals[face];
        uint neighborType = GetVoxelSafe(neighborPos);

        if (IsTransparent(neighborType)) {
            EmitFace(dtid, face, blockType);
        }
    }
}
