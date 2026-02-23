//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_VERTEX_H
#define DIRECTX11_VERTEX_H

#include <Core/Math/Vector2.h>
#include <Core/Math/Vector3.h>
#include <Core/Math/Vector4.h>

using Math::Vector2;
using Math::Vector3;
using Math::Vector4;

struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 texCoord;
    Vector4 color;

    // Optional: Tangent for normal mapping
    Vector3 tangent;

    // Optional: Light values for voxel lighting
    float lightLevel; // 0-15 for Minecraft-style lighting

    // Texture array slice index for multi-texture support
    uint32_t textureIndex;

    // UV rotation (0-3) for texture variation
    uint32_t uvRotation;

    Vertex()
        : position(0, 0, 0)
        , normal(0, 1, 0)
        , texCoord(0, 0)
        , color(1, 1, 1, 1)
        , tangent(1, 0, 0)
        , lightLevel(15.0f)
        , textureIndex(0)
        , uvRotation(0)
    {
    }

    Vertex(const Vector3& pos, const Vector3& norm, const Vector2& uv)
        : position(pos)
        , normal(norm)
        , texCoord(uv)
        , color(1, 1, 1, 1)
        , tangent(1, 0, 0)
        , lightLevel(15.0f)
        , textureIndex(0)
        , uvRotation(0)
    {
    }
};

// Compact voxel vertex (24 bytes) - used exclusively for chunk meshes
// Packed field layout (32 bits):
//   bits  0-2:  faceIndex    (0-5, which axis-aligned face normal)
//   bits  3-10: textureIndex (0-255, texture array slice)
//   bits 11-14: lightLevel   (0-15)
//   bits 15-16: uvRotation   (0-3)
//   bits 17-31: unused (reserved)
struct VoxelVertex {
    Vector3 position;    // 12 bytes - world space position
    Vector2 texCoord;    //  8 bytes - UV coordinates
    uint32_t packed;     //  4 bytes - packed normal/texIndex/light/uvRotation

    VoxelVertex()
        : position(0, 0, 0)
        , texCoord(0, 0)
        , packed(0)
    {
    }

    void Set(const Vector3& pos, const Vector2& uv,
             uint8_t faceIndex, uint32_t texIndex,
             uint8_t light, uint8_t uvRot)
    {
        position = pos;
        texCoord = uv;
        packed = (faceIndex & 0x7)
               | ((texIndex & 0xFF) << 3)
               | ((light & 0xF) << 11)
               | ((uvRot & 0x3) << 15);
    }
};

// Maximum bones per vertex (GPU standard — 4 is universally supported)
static constexpr int MAX_BONE_INFLUENCES = 4;
// Maximum bones in a skeleton (matches constant buffer array size in shader)
static constexpr int MAX_BONES = 128;
struct SkinnedVertex {
    Vector3  position;                         // 12 bytes  (offset 0)
    Vector3  normal;                           // 12 bytes  (offset 12)
    Vector2  texCoord;                         //  8 bytes  (offset 24)
    Vector4  color;                            // 16 bytes  (offset 32)
    Vector3  tangent;                          // 12 bytes  (offset 48)
    uint32_t boneIndices[MAX_BONE_INFLUENCES]; // 16 bytes  (offset 60)
    float    boneWeights[MAX_BONE_INFLUENCES]; // 16 bytes  (offset 76)
                                               // Total: 92 bytes

    SkinnedVertex()
        : position(0, 0, 0)
        , normal(0, 1, 0)
        , texCoord(0, 0)
        , color(1, 1, 1, 1)
        , tangent(1, 0, 0)
        , boneIndices{0, 0, 0, 0}
        , boneWeights{0.0f, 0.0f, 0.0f, 0.0f}
    {
    }

    SkinnedVertex(const Vector3& pos, const Vector3& norm, const Vector2& uv)
        : position(pos)
        , normal(norm)
        , texCoord(uv)
        , color(1, 1, 1, 1)
        , tangent(1, 0, 0)
        , boneIndices{0, 0, 0, 0}
        , boneWeights{0.0f, 0.0f, 0.0f, 0.0f}
    {
    }

    // Add a bone influence. Automatically fills the next available slot.
    // Returns false if all 4 slots are full.
    bool AddBoneInfluence(uint32_t boneIndex, float weight) {
        for (int i = 0; i < MAX_BONE_INFLUENCES; i++) {
            if (boneWeights[i] == 0.0f) {
                boneIndices[i] = boneIndex;
                boneWeights[i] = weight;
                return true;
            }
        }
        // All slots full — replace the smallest weight if this one is larger
        int minIdx = 0;
        for (int i = 1; i < MAX_BONE_INFLUENCES; i++) {
            if (boneWeights[i] < boneWeights[minIdx])
                minIdx = i;
        }
        if (weight > boneWeights[minIdx]) {
            boneIndices[minIdx] = boneIndex;
            boneWeights[minIdx] = weight;
        }
        return false;
    }

    // Normalize bone weights so they sum to 1.0
    void NormalizeBoneWeights() {
        float total = 0.0f;
        for (int i = 0; i < MAX_BONE_INFLUENCES; i++)
            total += boneWeights[i];

        if (total > 0.0f) {
            float inv = 1.0f / total;
            for (int i = 0; i < MAX_BONE_INFLUENCES; i++)
                boneWeights[i] *= inv;
        }
    }
};

#endif //DIRECTX11_VERTEX_H