//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_SHADERTYPES_H
#define DIRECTX11_SHADERTYPES_H

#pragma once
#include <cstdint>
#include "Core/Math/Matrix4x4.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Core/Math/Vector2.h"

enum class ShaderType {
    Vertex,
    Pixel,
    Geometry,
    Compute
};

struct alignas(16) PerFrameBuffer {
    Math::Matrix4x4 viewProjection;
    Math::Vector3 cameraPosition;
    float _pad;
};

struct alignas(16) PerObjectBuffer {
    Math::Matrix4x4 world;
    Math::Matrix4x4 worldInverseTranspose;
};

static constexpr uint32_t DRAW_ARGS_STRIDE = 20;  // 5 x uint32

struct ChunkGpuData {
    void* voxelBuffer = nullptr;      // Packed voxel data
    void* voxelSRV = nullptr;

    void* vertexBuffer = nullptr;     // Output vertices
    void* vertexSRV = nullptr;
    void* vertexUAV = nullptr;

    void* counterBuffer = nullptr;    // Atomic vertex counter
    void* counterUAV = nullptr;

    void* drawArgsBuffer = nullptr;   // Per-chunk indirect draw args
    void* drawArgsUAV = nullptr;

    Math::Vector3 worldPos;
    uint32_t faceCount = 0;           // Read back from GPU counter
    uint32_t chunkIndex = 0;          // Index into shared cull/draw buffers
    bool isDirty = true;
    bool isInitialized = false;
};

// GPU frustum culling data — matches FrustumCull.hlsl ChunkCullData (32 bytes)
struct ChunkCullData {
    float aabbMin[3];
    uint32_t drawArgsOffset;    // byte offset into shared draw args buffer
    float aabbMax[3];
    uint32_t faceCount;
};

// Constant buffer for frustum cull compute shader (112 bytes, 16-byte aligned)
struct alignas(16) FrustumCullConstants {
    Math::Vector4 frustumPlanes[6];
    uint32_t chunkCount;
    uint32_t _pad[3];
};

struct alignas(16) ChunkMeshConstants {
    float chunkWorldPos[3];
    uint32_t maxFaces;
};

struct alignas(16) ChunkPositionBuffer {
    int32_t chunkWorldPos[3];
    float voxelScale;
};

// Must match sky.vert.hlsl cbuffer SkyBuffer : register(b0)
struct SkyVertexConstants {
    Math::Matrix4x4 viewProjection;
    float cameraPosition[3];
    float time;
};

// Must match sky.pixel.hlsl cbuffer SkyPropertiesBuffer : register(b1)
struct SkyPropertiesConstants {
    float sunDirection[3];
    float sunIntensity;
    float sunColor[3];
    float sunSize;
    float moonDirection[3];
    float moonIntensity;
    float moonColor[3];
    float moonSize;

    float zenithColor[3];
    float timeOfDay;
    float horizonColor[3];
    float padding2;
    float nightZenithColor[3];
    float padding3;
    float nightHorizonColor[3];
    float starIntensity;

    float atmosphereThickness;
    float rayleighScattering;
    float mieScattering;
    float hasSkyboxTexture;
};

// Per-effect constant buffer — effects can extend this
struct PostProcessConstants {
    float screenWidth;
    float screenHeight;
    float time;
    float padding0;
    float cameraPosition[3];
    float padding1;
    float inverseViewProjection[16];  // For reconstructing world pos from depth
};


#endif //DIRECTX11_SHADERTYPES_H