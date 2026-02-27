//
// Created by Claude on 01/02/2026.
//

#ifndef VOXTAU_BLOCKREGISTRY_H
#define VOXTAU_BLOCKREGISTRY_H
#pragma once

#include <EngineApi.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

class TextureArray;
class TextureData;
class IRendererApi;

// Face indices for block textures
enum class BlockFace : uint8_t {
    Front = 0,   // +Z
    Back = 1,    // -Z
    Left = 2,    // -X
    Right = 3,   // +X
    Top = 4,     // +Y
    Bottom = 5   // -Y
};

// Block texture definition
struct BlockDefinition {
    uint8_t id;
    std::string name;
    bool transparent;
    bool randomRotation;  // Allow random UV rotation for texture variation

    // Texture indices for each face (index into texture array)
    uint32_t textureIndices[6];  // One per face

    BlockDefinition()
        : id(0)
        , transparent(false)
        , randomRotation(true)  // Enabled by default
    {
        for (int i = 0; i < 6; i++) {
            textureIndices[i] = 0;
        }
    }
};

class BlockRegistry {
public:
    BlockRegistry();
    ~BlockRegistry();

    // Load block definitions from JSON file
    bool LoadFromFile(const std::string& filepath);

    // Get texture index for a block type and face
    [[nodiscard]] uint32_t GetTextureIndex(uint8_t blockType, BlockFace face) const;
    [[nodiscard]] uint32_t GetTextureIndex(uint8_t blockType, int faceIndex) const;

    // Get block definition
    [[nodiscard]] const BlockDefinition* GetBlockDefinition(uint8_t blockType) const;
    [[nodiscard]] const BlockDefinition* GetBlockDefinitionByName(const std::string& name) const;

    // Check if block is transparent
    [[nodiscard]] bool IsTransparent(uint8_t blockType) const;

    // Check if block allows random UV rotation
    [[nodiscard]] bool AllowsRandomRotation(uint8_t blockType) const;

    // Get all texture paths (in order) for creating texture array
    [[nodiscard]] const std::vector<std::string>& GetTexturePaths() const;

    // Create the texture array from loaded definitions
    bool CreateTextureArray(IRendererApi* renderer, TextureArray* outTextureArray);

    // Get number of registered blocks
    [[nodiscard]] size_t GetBlockCount() const;

private:
    std::vector<std::string> texturePaths;
    std::unordered_map<std::string, uint32_t> textureNameToIndex;
    std::unordered_map<uint8_t, BlockDefinition> blockDefinitions;
    std::unordered_map<std::string, uint8_t> blockNameToId;

    std::string basePath;  // Base path for texture files

    uint32_t GetOrAddTextureIndex(const std::string& textureName);
};

#endif //VOXTAU_BLOCKREGISTRY_H
