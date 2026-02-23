//
// Created by Claude on 01/02/2026.
//

#include "BlockRegistry.h"
#include "TextureArray.h"
#include "TextureData.h"
#include "Renderer/RenderApi/IRendererApi.h"
#include "Renderer/RenderApi/DirectX11/DirectX11TextureArray.h"
#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>

using nlohmann::json;

BlockRegistry::BlockRegistry()
    : basePath("")
{
}

BlockRegistry::~BlockRegistry() {
}

bool BlockRegistry::LoadFromFile(const std::string& filepath) {
    // Extract base path from filepath
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        basePath = filepath.substr(0, lastSlash + 1);
    }

    // Read JSON file
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open blocks.json: " << filepath << std::endl;
        return false;
    }

    json root;
    try {
        file >> root;
    } catch (const json::parse_error& e) {
        std::cerr << "Failed to parse blocks.json: " << e.what() << std::endl;
        return false;
    }

    // Load texture list
    if (root.contains("textures") && root["textures"].is_array()) {
        for (const auto& texPath : root["textures"]) {
            if (texPath.is_string()) {
                std::string fullPath = texPath.get<std::string>();
                std::string texName = fullPath;

                // Extract just the filename for lookup
                size_t lastSlashTex = fullPath.find_last_of("/\\");
                if (lastSlashTex != std::string::npos) {
                    texName = fullPath.substr(lastSlashTex + 1);
                }

                uint32_t index = static_cast<uint32_t>(texturePaths.size());
                texturePaths.push_back(fullPath);
                textureNameToIndex[texName] = index;
            }
        }
    }

    // Load block definitions
    if (root.contains("blocks") && root["blocks"].is_object()) {
        for (auto& [name, blockJson] : root["blocks"].items()) {
            BlockDefinition def;
            def.name = name;

            if (blockJson.contains("id")) {
                def.id = static_cast<uint8_t>(blockJson["id"].get<int>());
            }

            if (blockJson.contains("transparent")) {
                def.transparent = blockJson["transparent"].get<bool>();
            }

            if (blockJson.contains("randomRotation")) {
                def.randomRotation = blockJson["randomRotation"].get<bool>();
            }

            // Handle single texture (applies to all faces)
            if (blockJson.contains("texture") && blockJson["texture"].is_string()) {
                std::string texName = blockJson["texture"].get<std::string>();
                uint32_t texIndex = GetOrAddTextureIndex(texName);
                for (int i = 0; i < 6; i++) {
                    def.textureIndices[i] = texIndex;
                }
            }

            // Handle per-face textures
            if (blockJson.contains("textures") && blockJson["textures"].is_object()) {
                auto& textures = blockJson["textures"];

                // Top face
                if (textures.contains("top")) {
                    def.textureIndices[static_cast<int>(BlockFace::Top)] =
                        GetOrAddTextureIndex(textures["top"].get<std::string>());
                }

                // Bottom face
                if (textures.contains("bottom")) {
                    def.textureIndices[static_cast<int>(BlockFace::Bottom)] =
                        GetOrAddTextureIndex(textures["bottom"].get<std::string>());
                }

                // Sides (front, back, left, right)
                if (textures.contains("sides")) {
                    uint32_t sideIndex = GetOrAddTextureIndex(textures["sides"].get<std::string>());
                    def.textureIndices[static_cast<int>(BlockFace::Front)] = sideIndex;
                    def.textureIndices[static_cast<int>(BlockFace::Back)] = sideIndex;
                    def.textureIndices[static_cast<int>(BlockFace::Left)] = sideIndex;
                    def.textureIndices[static_cast<int>(BlockFace::Right)] = sideIndex;
                }

                // Individual side overrides
                if (textures.contains("front")) {
                    def.textureIndices[static_cast<int>(BlockFace::Front)] =
                        GetOrAddTextureIndex(textures["front"].get<std::string>());
                }
                if (textures.contains("back")) {
                    def.textureIndices[static_cast<int>(BlockFace::Back)] =
                        GetOrAddTextureIndex(textures["back"].get<std::string>());
                }
                if (textures.contains("left")) {
                    def.textureIndices[static_cast<int>(BlockFace::Left)] =
                        GetOrAddTextureIndex(textures["left"].get<std::string>());
                }
                if (textures.contains("right")) {
                    def.textureIndices[static_cast<int>(BlockFace::Right)] =
                        GetOrAddTextureIndex(textures["right"].get<std::string>());
                }
            }

            blockDefinitions[def.id] = def;
            blockNameToId[name] = def.id;
        }
    }

    std::cout << "BlockRegistry: Loaded " << blockDefinitions.size() << " blocks and "
              << texturePaths.size() << " textures" << std::endl;

    return true;
}

uint32_t BlockRegistry::GetOrAddTextureIndex(const std::string& textureName) {
    // Check if already registered
    auto it = textureNameToIndex.find(textureName);
    if (it != textureNameToIndex.end()) {
        return it->second;
    }

    // Not found - this shouldn't happen if textures array is properly defined
    std::cerr << "Warning: Texture '" << textureName << "' not found in textures list!" << std::endl;
    return 0;  // Return first texture as fallback
}

uint32_t BlockRegistry::GetTextureIndex(uint8_t blockType, BlockFace face) const {
    return GetTextureIndex(blockType, static_cast<int>(face));
}

uint32_t BlockRegistry::GetTextureIndex(uint8_t blockType, int faceIndex) const {
    auto it = blockDefinitions.find(blockType);
    if (it != blockDefinitions.end()) {
        if (faceIndex >= 0 && faceIndex < 6) {
            return it->second.textureIndices[faceIndex];
        }
    }
    return 0;  // Default to first texture
}

const BlockDefinition* BlockRegistry::GetBlockDefinition(uint8_t blockType) const {
    auto it = blockDefinitions.find(blockType);
    if (it != blockDefinitions.end()) {
        return &it->second;
    }
    return nullptr;
}

const BlockDefinition* BlockRegistry::GetBlockDefinitionByName(const std::string& name) const {
    auto it = blockNameToId.find(name);
    if (it != blockNameToId.end()) {
        return GetBlockDefinition(it->second);
    }
    return nullptr;
}

bool BlockRegistry::IsTransparent(uint8_t blockType) const {
    auto it = blockDefinitions.find(blockType);
    if (it != blockDefinitions.end()) {
        return it->second.transparent;
    }
    return false;
}

bool BlockRegistry::AllowsRandomRotation(uint8_t blockType) const {
    auto it = blockDefinitions.find(blockType);
    if (it != blockDefinitions.end()) {
        return it->second.randomRotation;
    }
    return true;  // Default to allowing rotation
}

const std::vector<std::string>& BlockRegistry::GetTexturePaths() const {
    return texturePaths;
}

bool BlockRegistry::CreateTextureArray(IRendererApi* renderer, TextureArray* outTextureArray) {
    if (!renderer || !outTextureArray) {
        return false;
    }

    if (texturePaths.empty()) {
        std::cerr << "No textures to create array from!" << std::endl;
        return false;
    }

    // Load all textures
    std::vector<TextureData*> textureDataList;
    textureDataList.reserve(texturePaths.size());

    int expectedWidth = 0;
    int expectedHeight = 0;

    for (size_t i = 0; i < texturePaths.size(); i++) {
        TextureData* texData = new TextureData();
        if (!texData->LoadFromFile(texturePaths[i])) {
            std::cerr << "Failed to load texture: " << texturePaths[i] << std::endl;
            // Clean up
            for (auto* td : textureDataList) delete td;
            delete texData;
            return false;
        }

        // Verify dimensions match
        if (i == 0) {
            expectedWidth = texData->GetWidth();
            expectedHeight = texData->GetHeight();
        } else if (texData->GetWidth() != expectedWidth || texData->GetHeight() != expectedHeight) {
            std::cerr << "Texture " << texturePaths[i] << " has different dimensions! Expected "
                      << expectedWidth << "x" << expectedHeight << std::endl;
            for (auto* td : textureDataList) delete td;
            delete texData;
            return false;
        }

        // Convert to RGBA if needed
        texData->ConvertToRGBA();

        textureDataList.push_back(texData);
    }

    // Create DirectX11 texture array
    DirectX11TextureArray* d3dTexArray = new DirectX11TextureArray();
    bool success = d3dTexArray->Create(
        renderer->GetDevice(),
        renderer->GetContext(),
        textureDataList,
        true  // Generate mipmaps
    );

    // Clean up texture data
    for (auto* td : textureDataList) {
        delete td;
    }

    if (!success) {
        delete d3dTexArray;
        return false;
    }

    // Store in high-level wrapper
    outTextureArray->SetGPUHandle(d3dTexArray);
    outTextureArray->SetDimensions(expectedWidth, expectedHeight, static_cast<int>(texturePaths.size()));
    outTextureArray->SetTextureFilepaths(texturePaths);

    return true;
}

size_t BlockRegistry::GetBlockCount() const {
    return blockDefinitions.size();
}
