//
// Created by Marvin on 10/02/2026.
//

#ifndef VOXTAU_VOXLOADER_H
#define VOXTAU_VOXLOADER_H
#pragma once
#include <string>
#include <EngineApi.h>
#include "VoxModel.h"

class ENGINE_API VoxLoader {
public:
    // Load a .vox file from disk. Returns true on success.
    static bool Load(const std::string& filepath, VoxModel& outModel);

private:
    // Read helpers
    static uint32_t ReadU32(const uint8_t* data, size_t& offset);
    static uint8_t ReadU8(const uint8_t* data, size_t& offset);
    static bool ReadChunkHeader(const uint8_t* data, size_t& offset, char outId[5],
                                uint32_t& outContentSize, uint32_t& outChildrenSize);
    static void SetDefaultPalette(VoxModel& model);
};


#endif //VOXTAU_VOXLOADER_H