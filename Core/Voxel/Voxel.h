//
// Created by Marvin on 04/02/2026.
//

#ifndef VOXTAU_VOXEL_H
#define VOXTAU_VOXEL_H
#include "EngineApi.h"
#include "Core/Scene/Transform.h"


class ENGINE_API Voxel
{
public:
    Transform transform;

    Voxel();
    ~Voxel();
    bool IsActive();
    void SetActive(bool active);
private:
    bool _active;
};


#endif //VOXTAU_VOXEL_H