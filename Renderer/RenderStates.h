//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_RENDERSTATES_H
#define DIRECTX11_RENDERSTATES_H

#pragma once

// Rasterizer modes
enum class RasterizerMode {
    Solid,          // Normal rendering (filled triangles)
    Wireframe,      // Debug wireframe
    NoCull,         // No backface culling (render both sides)
    CullFront,      // Cull front faces
    CullBack        // Cull back faces (default)
};

// Blend modes
enum class BlendMode {
    None,           // No blending (opaque)
    Alpha,          // Standard alpha blending (transparency)
    Additive,       // Additive blending (glowing effects)
    Multiply,       // Multiplicative blending
    Premultiplied   // Premultiplied alpha
};

// Depth test modes
enum class DepthMode {
    None,           // No depth testing
    Less,           // Default (closer objects in front)
    LessEqual,
    Greater,
    GreaterEqual,
    Equal,
    NotEqual,
    Always,
    ReadOnly        // Test depth but don't write (for occlusion queries)
};

// Cull modes
enum class CullMode {
    None,           // No culling
    Front,          // Cull front faces
    Back            // Cull back faces (default)
};

// Fill modes
enum class FillMode {
    Solid,          // Fill triangles
    Wireframe       // Wireframe only
};

// Texture filter modes
enum class TextureFilter {
    Point,          // Nearest neighbor (pixelated - good for voxels)
    Linear,         // Bilinear filtering (smooth)
    Anisotropic     // Anisotropic filtering (high quality)
};

// Texture address modes
enum class TextureWrap {
    Repeat,         // Tile texture
    Clamp,          // Clamp to edge
    Mirror,         // Mirror texture
    Border          // Use border color
};

#endif //DIRECTX11_RENDERSTATES_H