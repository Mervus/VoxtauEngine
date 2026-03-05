//
// Created by Marvin on 10/02/2026.
//

#ifndef VOXTAU_VOXMESHER_H
#define VOXTAU_VOXMESHER_H
#pragma once
#include <EngineApi.h>
#include "VoxModel.h"

struct Vertex;
class Mesh;

class VoxMesher {
public:
    // Convert a VoxModel to a Mesh with per-face vertex colors from the palette.
    // centerOrigin: if true, centers the mesh so (0,0,0) is the model center.
    //               if false, origin is at the model's (0,0,0) corner.
    // scale: size of each voxel in world units (default 1.0 = 1 voxel = 1 unit)
    static Mesh* CreateMesh(const VoxModel& model, const std::string& name,
                            bool centerOrigin = true, float scale = 1.0f);

private:
    // Check if a voxel exists at (x, y, z) in the model
    static bool IsSolid(const uint8_t* grid, uint32_t sx, uint32_t sy, uint32_t sz,
                        int x, int y, int z);

    // Add a quad face (two triangles) with the given color
    static void AddFace(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                        const Math::Vector3& p0, const Math::Vector3& p1,
                        const Math::Vector3& p2, const Math::Vector3& p3,
                        const Math::Vector3& normal, const VoxColor& color);
};



#endif //VOXTAU_VOXMESHER_H