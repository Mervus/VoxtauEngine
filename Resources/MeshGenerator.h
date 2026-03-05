//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_MESHGENERATOR_H
#define DIRECTX11_MESHGENERATOR_H

#pragma once
#include "Mesh.h"
#include "../Renderer/Vertex.h"
#include "../Core/Math/Vector2.h"
#include "../Core/Math/Vector3.h"
#include "../Core/Math/Vector4.h"
#include <EngineApi.h>

class MeshGenerator {
public:
    // Create a simple cube (1x1x1)
    static Mesh* CreateCube(const Math::Vector3& position = Math::Vector3(0, 0, 0));

    // Create a textured cube with proper UVs
    static Mesh* CreateTexturedCube(const Math::Vector3& position, float size = 1.0f);

    static Mesh* CreateSphere(float radius = 1.0f, int segments = 32, int rings = 32);

    // Create a fullscreen quad for post-processing (covers NDC -1 to 1)
    static Mesh* CreateFullscreenQuad();
};


#endif //DIRECTX11_MESHGENERATOR_H