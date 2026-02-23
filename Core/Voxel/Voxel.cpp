//
// Created by Marvin on 04/02/2026.
//

#include "Voxel.h"

Voxel::Voxel() : _active(true)
{
}

Voxel::~Voxel()
{
}

bool Voxel::IsActive()
{
    return _active;
}

void Voxel::SetActive(bool active)
{
    _active = active;
}
