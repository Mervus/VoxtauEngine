// ChunkMesh.hlsl — Generate mesh faces for a 32x32x32 chunk
//
// Voxel data is a PADDED 34x34x34 grid: the 32³ chunk data surrounded by
// a 1-block border sampled from neighbor chunks on the CPU.
// This means all boundary lookups (face culling + AO) just read from the
// same buffer — no separate neighbor SRVs, no ordering issues.
//
// Output: 2 packed uints per visible face
//   [0] = position + face + texIndex
//   [1] = 4 corner AO values (bits [7:0]) + flip bit [8]

// Padded voxel data: 34³ = 39304 voxels, packed 4 per uint = 9826 uints
// Layout: padded index = (x+1) + (z+1)*34 + (y+1)*34*34
StructuredBuffer<uint> VoxelData : register(t0);

// Block texture lookup
StructuredBuffer<uint> BlockTextureMap : register(t7);

// Output faces
RWStructuredBuffer<uint> OutputFaces : register(u0);
RWByteAddressBuffer FaceCounter : register(u1);

cbuffer MeshConstants : register(b0) {
    float3 ChunkWorldPos;
    uint   MaxFaces;
};

// Padded grid access
static const uint PAD = 34;

uint GetVoxel(int3 p) {
    // p is local chunk coords: -1 to 32 (border included)
    // Offset into padded grid: +1 on each axis
    uint3 q = uint3(p.x + 1, p.y + 1, p.z + 1);
    uint flat = q.x + q.z * PAD + q.y * PAD * PAD;
    uint word = VoxelData[flat / 4];
    uint shift = (flat % 4) * 8;
    return (word >> shift) & 0xFF;
}

bool IsTransparent(uint blockType) {
    return blockType == 0;
}

bool IsSolid(int3 p) {
    return !IsTransparent(GetVoxel(p));
}

// Packing
uint PackFace(uint3 pos, uint faceIdx, uint texIndex) {
    return (pos.x & 0x1F)
         | ((pos.y & 0x1F) << 5)
         | ((pos.z & 0x1F) << 10)
         | ((faceIdx & 0x7) << 15)
         | ((texIndex & 0xFF) << 18);
}

static const int3 FaceNormals[6] = {
    int3( 1,  0,  0),
    int3(-1,  0,  0),
    int3( 0,  1,  0),
    int3( 0, -1,  0),
    int3( 0,  0,  1),
    int3( 0,  0, -1),
};

// AO
uint VertexAO(bool side1, bool side2, bool corner) {
    if (side1 && side2) return 0;
    return 3 - (uint(side1) + uint(side2) + uint(corner));
}

uint ComputeFaceAO(int3 pos, uint faceIdx) {
    //return (3) | (3 << 2) | (3 << 4) | (3 << 6); //Turn AO off.
    int3 n = FaceNormals[faceIdx];
    int3 airPos = pos + n;

    uint ao0, ao1, ao2, ao3;

    switch (faceIdx) {
        case 0: { // +X face: corners at (1,0,0),(1,1,0),(1,1,1),(1,0,1)
            bool s_yN = IsSolid(airPos + int3(0,-1, 0));
            bool s_yP = IsSolid(airPos + int3(0, 1, 0));
            bool s_zN = IsSolid(airPos + int3(0, 0,-1));
            bool s_zP = IsSolid(airPos + int3(0, 0, 1));
            bool c_yNzN = IsSolid(airPos + int3(0,-1,-1));
            bool c_yPzN = IsSolid(airPos + int3(0, 1,-1));
            bool c_yPzP = IsSolid(airPos + int3(0, 1, 1));
            bool c_yNzP = IsSolid(airPos + int3(0,-1, 1));
            ao0 = VertexAO(s_yN, s_zN, c_yNzN);
            ao1 = VertexAO(s_yP, s_zN, c_yPzN);
            ao2 = VertexAO(s_yP, s_zP, c_yPzP);
            ao3 = VertexAO(s_yN, s_zP, c_yNzP);
            break;
        }
        case 1: { // -X face: corners at (0,0,1),(0,1,1),(0,1,0),(0,0,0)
            bool s_yN = IsSolid(airPos + int3(0,-1, 0));
            bool s_yP = IsSolid(airPos + int3(0, 1, 0));
            bool s_zN = IsSolid(airPos + int3(0, 0,-1));
            bool s_zP = IsSolid(airPos + int3(0, 0, 1));
            bool c_yNzP = IsSolid(airPos + int3(0,-1, 1));
            bool c_yPzP = IsSolid(airPos + int3(0, 1, 1));
            bool c_yPzN = IsSolid(airPos + int3(0, 1,-1));
            bool c_yNzN = IsSolid(airPos + int3(0,-1,-1));
            ao0 = VertexAO(s_yN, s_zP, c_yNzP); // (0,0,1)
            ao1 = VertexAO(s_yP, s_zP, c_yPzP); // (0,1,1)
            ao2 = VertexAO(s_yP, s_zN, c_yPzN); // (0,1,0)
            ao3 = VertexAO(s_yN, s_zN, c_yNzN); // (0,0,0)
            break;
        }
        case 2: { // +Y face: corners at (0,1,0),(0,1,1),(1,1,1),(1,1,0)
            bool s_xN = IsSolid(airPos + int3(-1, 0, 0));
            bool s_xP = IsSolid(airPos + int3( 1, 0, 0));
            bool s_zN = IsSolid(airPos + int3( 0, 0,-1));
            bool s_zP = IsSolid(airPos + int3( 0, 0, 1));
            bool c_xNzN = IsSolid(airPos + int3(-1, 0,-1));
            bool c_xNzP = IsSolid(airPos + int3(-1, 0, 1));
            bool c_xPzP = IsSolid(airPos + int3( 1, 0, 1));
            bool c_xPzN = IsSolid(airPos + int3( 1, 0,-1));
            ao0 = VertexAO(s_xN, s_zN, c_xNzN); // (0,1,0)
            ao1 = VertexAO(s_xN, s_zP, c_xNzP); // (0,1,1)
            ao2 = VertexAO(s_xP, s_zP, c_xPzP); // (1,1,1)
            ao3 = VertexAO(s_xP, s_zN, c_xPzN); // (1,1,0)
            break;
        }
        case 3: { // -Y face: corners at (0,0,1),(0,0,0),(1,0,0),(1,0,1)
            bool s_xN = IsSolid(airPos + int3(-1, 0, 0));
            bool s_xP = IsSolid(airPos + int3( 1, 0, 0));
            bool s_zN = IsSolid(airPos + int3( 0, 0,-1));
            bool s_zP = IsSolid(airPos + int3( 0, 0, 1));
            bool c_xNzP = IsSolid(airPos + int3(-1, 0, 1));
            bool c_xNzN = IsSolid(airPos + int3(-1, 0,-1));
            bool c_xPzN = IsSolid(airPos + int3( 1, 0,-1));
            bool c_xPzP = IsSolid(airPos + int3( 1, 0, 1));
            ao0 = VertexAO(s_xN, s_zP, c_xNzP); // (0,0,1)
            ao1 = VertexAO(s_xN, s_zN, c_xNzN); // (0,0,0)
            ao2 = VertexAO(s_xP, s_zN, c_xPzN); // (1,0,0)
            ao3 = VertexAO(s_xP, s_zP, c_xPzP); // (1,0,1)
            break;
        }
        case 4: { // +Z face: corners at (0,0,1),(1,0,1),(1,1,1),(0,1,1)
            bool s_xN = IsSolid(airPos + int3(-1, 0, 0));
            bool s_xP = IsSolid(airPos + int3( 1, 0, 0));
            bool s_yN = IsSolid(airPos + int3( 0,-1, 0));
            bool s_yP = IsSolid(airPos + int3( 0, 1, 0));
            bool c_xNyN = IsSolid(airPos + int3(-1,-1, 0));
            bool c_xPyN = IsSolid(airPos + int3( 1,-1, 0));
            bool c_xPyP = IsSolid(airPos + int3( 1, 1, 0));
            bool c_xNyP = IsSolid(airPos + int3(-1, 1, 0));
            ao0 = VertexAO(s_xN, s_yN, c_xNyN); // (0,0,1)
            ao1 = VertexAO(s_xP, s_yN, c_xPyN); // (1,0,1)
            ao2 = VertexAO(s_xP, s_yP, c_xPyP); // (1,1,1)
            ao3 = VertexAO(s_xN, s_yP, c_xNyP); // (0,1,1)
            break;
        }
        case 5: { // -Z face: corners at (1,0,0),(0,0,0),(0,1,0),(1,1,0)
            bool s_xN = IsSolid(airPos + int3(-1, 0, 0));
            bool s_xP = IsSolid(airPos + int3( 1, 0, 0));
            bool s_yN = IsSolid(airPos + int3( 0,-1, 0));
            bool s_yP = IsSolid(airPos + int3( 0, 1, 0));
            bool c_xPyN = IsSolid(airPos + int3( 1,-1, 0));
            bool c_xNyN = IsSolid(airPos + int3(-1,-1, 0));
            bool c_xNyP = IsSolid(airPos + int3(-1, 1, 0));
            bool c_xPyP = IsSolid(airPos + int3( 1, 1, 0));
            ao0 = VertexAO(s_xP, s_yN, c_xPyN); // (1,0,0)
            ao1 = VertexAO(s_xN, s_yN, c_xNyN); // (0,0,0)
            ao2 = VertexAO(s_xN, s_yP, c_xNyP); // (0,1,0)
            ao3 = VertexAO(s_xP, s_yP, c_xPyP); // (1,1,0)
            break;
        }
        default:
            ao0 = ao1 = ao2 = ao3 = 3;
            break;
    }

    uint flipBit = ((ao0 + ao2) < (ao1 + ao3)) ? 1 : 0;
    return (ao0 & 0x3) | ((ao1 & 0x3) << 2) | ((ao2 & 0x3) << 4) | ((ao3 & 0x3) << 6) | (flipBit << 8);
}

// Emit
void EmitFace(uint3 voxelPos, uint faceIdx, uint blockType) {
    uint baseFace;
    FaceCounter.InterlockedAdd(0, 1, baseFace);
    if (baseFace >= MaxFaces) return;

    uint texIndex = BlockTextureMap[blockType * 6 + faceIdx];
    uint aoData = ComputeFaceAO(int3(voxelPos), faceIdx);

    OutputFaces[baseFace * 2 + 0] = PackFace(voxelPos, faceIdx, texIndex);
    OutputFaces[baseFace * 2 + 1] = aoData;
}

// Main
[numthreads(8, 8, 8)]
void main(uint3 dtid : SV_DispatchThreadID) {
    if (any(dtid >= 32)) return;

    uint blockType = GetVoxel(int3(dtid));
    if (IsTransparent(blockType)) return;

    [unroll]
    for (uint face = 0; face < 6; face++) {
        int3 neighborPos = int3(dtid) + FaceNormals[face];
        // neighborPos can be -1 or 32 — reads border data from padded grid
        if (IsTransparent(GetVoxel(neighborPos))) {
            EmitFace(dtid, face, blockType);
        }
    }
}