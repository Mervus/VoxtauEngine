//
// Created by Marvin on 14/02/2026.
//

#include "ModelLoader.h"
#include "Resources/SkinnedMesh.h"
#include "Resources/Mesh.h"
#include "Resources/TextureData.h"
#include "Resources/Animation/Skeleton.h"
#include "Resources/Animation/AnimationClip.h"
#include "Renderer/Vertex.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <unordered_map>
#include <chrono>
#include <filesystem>

// Helper: Convert Assimp types to engine types
static Math::Vector3 ToVec3(const aiVector3D& v) {
    return Math::Vector3(v.x, v.y, v.z);
}

static Math::Vector2 ToVec2(const aiVector3D& v) {
    return Math::Vector2(v.x, v.y);
}

static Math::Quaternion ToQuat(const aiQuaternion& q) {
    return Math::Quaternion(q.x, q.y, q.z, q.w);
}

static Math::Matrix4x4 ToMat4(const aiMatrix4x4& m) {
    // Assimp aiMatrix4x4 is row-major, column-vector convention:
    //   [a1 a2 a3 a4]     row 0 = right/X basis
    //   [b1 b2 b3 b4]     row 1 = up/Y basis
    //   [c1 c2 c3 c4]     row 2 = forward/Z basis
    //   [d1 d2 d3 d4]     row 3 = translation
    //
    // Our engine is row-major, row-vector convention (v * M):
    //   Translation lives in row 3 (m[3][0], m[3][1], m[3][2])
    //   Same storage, BUT our multiplication is transposed relative to Assimp's.
    //   So we transpose: our m[row][col] = assimp's m[col][row]
    Math::Matrix4x4 result;
    result.m[0][0] = m.a1; result.m[0][1] = m.b1; result.m[0][2] = m.c1; result.m[0][3] = m.d1;
    result.m[1][0] = m.a2; result.m[1][1] = m.b2; result.m[1][2] = m.c2; result.m[1][3] = m.d2;
    result.m[2][0] = m.a3; result.m[2][1] = m.b3; result.m[2][2] = m.c3; result.m[2][3] = m.d3;
    result.m[3][0] = m.a4; result.m[3][1] = m.b4; result.m[3][2] = m.c4; result.m[3][3] = m.d4;
    return result;
}

// Helper: Recursively build skeleton from Assimp node tree
//
// KEY INSIGHT: Assimp animation channels reference NODES, not just bones.
// We must track ALL nodes in the hierarchy so that animations play correctly.
// The skeleton stores only the actual bone entries, but the Animator needs
// to walk the full node tree.
//
// Strategy: We store every node that is either a bone OR an ancestor of a bone
// as a "joint" in our skeleton. The inverse bind pose is identity for non-bone
// nodes (they don't directly deform vertices, but they affect the hierarchy).

static void CollectBoneAncestors(
    const aiNode* node,
    const std::unordered_map<std::string, aiMatrix4x4>& boneOffsetMap,
    std::unordered_set<std::string>& relevantNodes)
{
    std::string name(node->mName.C_Str());

    // If this node is a bone, mark it and all ancestors as relevant
    if (boneOffsetMap.count(name)) {
        const aiNode* current = node;
        while (current) {
            std::string cName(current->mName.C_Str());
            if (relevantNodes.count(cName)) break; // already marked up the chain
            relevantNodes.insert(cName);
            current = current->mParent;
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
        CollectBoneAncestors(node->mChildren[i], boneOffsetMap, relevantNodes);
}

static void BuildSkeletonRecursive(
    const aiNode* node,
    int parentIndex,
    Skeleton& skeleton,
    const std::unordered_map<std::string, aiMatrix4x4>& boneOffsetMap,
    const std::unordered_set<std::string>& relevantNodes)
{
    std::string nodeName(node->mName.C_Str());

    // Skip nodes that aren't relevant to the skeleton
    if (!relevantNodes.count(nodeName)) {
        for (unsigned int i = 0; i < node->mNumChildren; i++)
            BuildSkeletonRecursive(node->mChildren[i], parentIndex, skeleton,
                                   boneOffsetMap, relevantNodes);
        return;
    }

    // Decompose the node's local transform
    aiVector3D aiPos, aiScale;
    aiQuaternion aiRot;
    node->mTransformation.Decompose(aiScale, aiRot, aiPos);

    // If this is an actual bone, use its offset matrix. Otherwise use identity.
    Math::Matrix4x4 inverseBindPose = Math::Matrix4x4::Identity;
    auto it = boneOffsetMap.find(nodeName);
    if (it != boneOffsetMap.end()) {
        inverseBindPose = ToMat4(it->second);
    }

    int boneIndex = skeleton.AddBone(
        nodeName,
        parentIndex,
        ToVec3(aiPos),
        ToQuat(aiRot),
        ToVec3(aiScale),
        inverseBindPose
    );

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        BuildSkeletonRecursive(node->mChildren[i], boneIndex, skeleton,
                               boneOffsetMap, relevantNodes);
    }
}

// Helper: Extract animation clips from a scene

static std::vector<std::shared_ptr<AnimationClip>>
ExtractAnimations(const aiScene* scene, const std::string& filepath) {
    std::vector<std::shared_ptr<AnimationClip>> clips;

    for (unsigned int a = 0; a < scene->mNumAnimations; a++) {
        const aiAnimation* aiAnim = scene->mAnimations[a];

        auto clip = std::make_shared<AnimationClip>();
        clip->name = aiAnim->mName.C_Str();
        clip->duration = static_cast<float>(aiAnim->mDuration);
        clip->ticksPerSecond = (aiAnim->mTicksPerSecond > 0.0)
            ? static_cast<float>(aiAnim->mTicksPerSecond)
            : 24.0f; // Mixamo default

        // Mixamo names every clip "mixamo.com" — useless.
        // Derive a proper name from the filename instead.
        if (clip->name.empty() || clip->name == "mixamo.com") {
            size_t lastSlash = filepath.find_last_of("/\\");
            size_t lastDot = filepath.find_last_of('.');
            if (lastSlash == std::string::npos) lastSlash = 0; else lastSlash++;
            if (lastDot == std::string::npos) lastDot = filepath.size();
            clip->name = filepath.substr(lastSlash, lastDot - lastSlash);
        }

        // Process each bone channel
        for (unsigned int c = 0; c < aiAnim->mNumChannels; c++) {
            const aiNodeAnim* nodeAnim = aiAnim->mChannels[c];

            BoneAnimation boneAnim;
            boneAnim.boneName = nodeAnim->mNodeName.C_Str();

            // Position keys
            boneAnim.positionKeys.reserve(nodeAnim->mNumPositionKeys);
            for (unsigned int k = 0; k < nodeAnim->mNumPositionKeys; k++) {
                PositionKey key;
                key.time = static_cast<float>(nodeAnim->mPositionKeys[k].mTime);
                key.value = ToVec3(nodeAnim->mPositionKeys[k].mValue);
                boneAnim.positionKeys.push_back(key);
            }

            // Rotation keys
            boneAnim.rotationKeys.reserve(nodeAnim->mNumRotationKeys);
            for (unsigned int k = 0; k < nodeAnim->mNumRotationKeys; k++) {
                RotationKey key;
                key.time = static_cast<float>(nodeAnim->mRotationKeys[k].mTime);
                key.value = ToQuat(nodeAnim->mRotationKeys[k].mValue);
                boneAnim.rotationKeys.push_back(key);
            }

            // Scale keys
            boneAnim.scaleKeys.reserve(nodeAnim->mNumScalingKeys);
            for (unsigned int k = 0; k < nodeAnim->mNumScalingKeys; k++) {
                ScaleKey key;
                key.time = static_cast<float>(nodeAnim->mScalingKeys[k].mTime);
                key.value = ToVec3(nodeAnim->mScalingKeys[k].mValue);
                boneAnim.scaleKeys.push_back(key);
            }

            clip->AddChannel(boneAnim);
        }

        clips.push_back(clip);

        std::cout << "ModelLoader: Loaded animation '" << clip->name
                  << "' (" << clip->channels.size() << " channels, "
                  << clip->GetDurationSeconds() << "s)" << std::endl;
    }
    return clips;
}

// LoadSkinnedModel mesh + skeleton + embedded animations
SkinnedModelData ModelLoader::LoadSkinnedModel(const std::string& filepath) {
    SkinnedModelData result;

    auto t0 = std::chrono::high_resolution_clock::now();
    std::cout << "ModelLoader: Reading file " << filepath << " ..." << std::endl;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_LimitBoneWeights |      // Limits to 4 bones per vertex
        aiProcess_FlipUVs                 // FBX uses opposite UV convention
    );

    auto t1 = std::chrono::high_resolution_clock::now();
    auto readMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "ModelLoader: ReadFile completed in " << readMs << " ms" << std::endl;

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ModelLoader: Failed to load: " << filepath
                  << " — " << importer.GetErrorString() << std::endl;
        return result;
    }

    // Log scene stats
    uint32_t totalVerts = 0, totalFaces = 0;
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        totalVerts += scene->mMeshes[m]->mNumVertices;
        totalFaces += scene->mMeshes[m]->mNumFaces;
    }
    std::cout << "ModelLoader: Scene has " << scene->mNumMeshes << " meshes, "
              << totalVerts << " vertices, " << totalFaces << " faces" << std::endl;

    //  Step 1: Collect all bone offset matrices across all meshes
    auto tStep = std::chrono::high_resolution_clock::now();
    std::unordered_map<std::string, aiMatrix4x4> boneOffsetMap;

    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        const aiMesh* aiMesh = scene->mMeshes[m];
        for (unsigned int b = 0; b < aiMesh->mNumBones; b++) {
            const aiBone* bone = aiMesh->mBones[b];
            std::string boneName(bone->mName.C_Str());
            boneOffsetMap[boneName] = bone->mOffsetMatrix;
        }
    }

    std::cout << "ModelLoader: Found " << boneOffsetMap.size() << " bones" << std::endl;

    //  Step 2: Build skeleton from node hierarchy
    auto skeleton = std::make_shared<Skeleton>();

    if (!boneOffsetMap.empty()) {
        // First pass: find all nodes relevant to the skeleton
        // (bones + their ancestors up to root)
        std::unordered_set<std::string> relevantNodes;
        CollectBoneAncestors(scene->mRootNode, boneOffsetMap, relevantNodes);

        // Second pass: build the skeleton from relevant nodes only
        BuildSkeletonRecursive(scene->mRootNode, -1,
                               *skeleton, boneOffsetMap, relevantNodes);
        std::cout << "ModelLoader: Built skeleton with " << skeleton->GetBoneCount()
                  << " bones" << std::endl;
    }

    result.skeleton = skeleton;

    auto tSkel = std::chrono::high_resolution_clock::now();
    auto skelMs = std::chrono::duration_cast<std::chrono::milliseconds>(tSkel - tStep).count();
    std::cout << "ModelLoader: Skeleton built in " << skelMs << " ms" << std::endl;

    //  Step 3: Load mesh data
    // Combine all meshes in the scene into one SkinnedMesh
    std::vector<SkinnedVertex> allVertices;
    std::vector<uint32_t> allIndices;
    uint32_t vertexOffset = 0;

    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        const aiMesh* mesh = scene->mMeshes[m];

        // Reserve space
        size_t baseVertex = allVertices.size();
        allVertices.resize(baseVertex + mesh->mNumVertices);

        // Fill vertex data
        for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
            SkinnedVertex& vert = allVertices[baseVertex + v];

            vert.position = ToVec3(mesh->mVertices[v]);

            if (mesh->mNormals)
                vert.normal = ToVec3(mesh->mNormals[v]);

            if (mesh->mTextureCoords[0])
                vert.texCoord = ToVec2(mesh->mTextureCoords[0][v]);

            if (mesh->mTangents)
                vert.tangent = ToVec3(mesh->mTangents[v]);

            if (mesh->mColors[0]) {
                vert.color = Math::Vector4(
                    mesh->mColors[0][v].r,
                    mesh->mColors[0][v].g,
                    mesh->mColors[0][v].b,
                    mesh->mColors[0][v].a
                );
            }

            // Bone data is filled below
        }

        // Fill bone weights
        for (unsigned int b = 0; b < mesh->mNumBones; b++) {
            const aiBone* bone = mesh->mBones[b];
            std::string boneName(bone->mName.C_Str());
            int boneIndex = skeleton->GetBoneIndex(boneName);

            if (boneIndex < 0) {
                // This shouldn't happen if skeleton was built correctly
                std::cerr << "ModelLoader: Warning — bone '" << boneName
                          << "' not found in skeleton" << std::endl;
                continue;
            }

            for (unsigned int w = 0; w < bone->mNumWeights; w++) {
                uint32_t vertIdx = static_cast<uint32_t>(baseVertex) + bone->mWeights[w].mVertexId;
                float weight = bone->mWeights[w].mWeight;
                allVertices[vertIdx].AddBoneInfluence(
                    static_cast<uint32_t>(boneIndex), weight);
            }
        }

        // Fill indices
        for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
            const aiFace& face = mesh->mFaces[f];
            for (unsigned int i = 0; i < face.mNumIndices; i++) {
                allIndices.push_back(vertexOffset + face.mIndices[i]);
            }
        }

        vertexOffset += mesh->mNumVertices;
    }

    // Normalize bone weights for all vertices
    for (auto& v : allVertices)
        v.NormalizeBoneWeights();

    auto tMesh = std::chrono::high_resolution_clock::now();
    auto meshMs = std::chrono::duration_cast<std::chrono::milliseconds>(tMesh - tSkel).count();
    std::cout << "ModelLoader: Mesh data processed in " << meshMs << " ms" << std::endl;

    // Create SkinnedMesh
    auto* skinnedMesh = new SkinnedMesh(filepath);
    skinnedMesh->SetSkinnedData(allVertices, allIndices);
    result.mesh = skinnedMesh;

    std::cout << "ModelLoader: Loaded mesh from " << filepath
              << " (" << allVertices.size() << " vertices, "
              << allIndices.size() / 3 << " triangles, "
              << skeleton->GetBoneCount() << " bones)" << std::endl;

    // Step 4: Extract embedded animations
    result.animations = ExtractAnimations(scene, filepath);
    return result;
}

// LoadAnimations — animation-only FBX (Mixamo additional clips)
std::vector<std::shared_ptr<AnimationClip>>
ModelLoader::LoadAnimations(const std::string& filepath,
                              const std::shared_ptr<Skeleton>& skeleton) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath,
        aiProcess_Triangulate // minimal processing — we only want animations
    );

    if (!scene) {
        std::cerr << "ModelLoader: Failed to load animations from: " << filepath
                  << " — " << importer.GetErrorString() << std::endl;
        return {};
    }

    if (scene->mNumAnimations == 0) {
        std::cerr << "ModelLoader: No animations found in: " << filepath << std::endl;
        return {};
    }

    auto clips = ExtractAnimations(scene, filepath);

    // Validate that the animation bones match the skeleton
    for (const auto& clip : clips) {
        int matched = 0;
        for (const auto& channel : clip->channels) {
            if (skeleton->GetBoneIndex(channel.boneName) >= 0)
                matched++;
        }
        std::cout << "ModelLoader: Animation '" << clip->name
                  << "' — " << matched << "/" << clip->channels.size()
                  << " channels matched skeleton" << std::endl;
    }

    return clips;
}

// LoadStaticMesh — non-skinned mesh via Assimp (replaces OBJ loader)
Mesh* ModelLoader::LoadStaticMesh(const std::string& filepath) {
    auto t0 = std::chrono::high_resolution_clock::now();
    std::cout << "ModelLoader: Reading static mesh " << filepath << " ..." << std::endl;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_FlipUVs
    );

    auto t1 = std::chrono::high_resolution_clock::now();
    auto readMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "ModelLoader: ReadFile completed in " << readMs << " ms" << std::endl;

    for (unsigned int i = 0; i < scene->mNumTextures; i++)
        std::cout << "  Embedded texture: " << scene->mTextures[i]->mFilename.C_Str() <<
    std::endl;

    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiString path;
        if (scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &path))
            std::cout << "  Material diffuse: " << path.C_Str() << std::endl;
    }

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ModelLoader: Failed to load static mesh: " << filepath
                  << " — " << importer.GetErrorString() << std::endl;
        return nullptr;
    }

    std::vector<Vertex> allVertices;
    std::vector<uint32_t> allIndices;
    uint32_t vertexOffset = 0;

    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        const aiMesh* mesh = scene->mMeshes[m];

        for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
            Vertex vert;
            vert.position = ToVec3(mesh->mVertices[v]);
            if (mesh->mNormals)
                vert.normal = ToVec3(mesh->mNormals[v]);
            if (mesh->mTextureCoords[0])
                vert.texCoord = ToVec2(mesh->mTextureCoords[0][v]);
            if (mesh->mTangents)
                vert.tangent = ToVec3(mesh->mTangents[v]);
            if (mesh->mColors[0]) {
                vert.color = Math::Vector4(
                    mesh->mColors[0][v].r, mesh->mColors[0][v].g,
                    mesh->mColors[0][v].b, mesh->mColors[0][v].a);
            }
            allVertices.push_back(vert);
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
            const aiFace& face = mesh->mFaces[f];
            for (unsigned int i = 0; i < face.mNumIndices; i++)
                allIndices.push_back(vertexOffset + face.mIndices[i]);
        }

        vertexOffset += mesh->mNumVertices;
    }

    auto* outMesh = new Mesh(filepath);
    outMesh->SetData(allVertices, allIndices);

    std::cout << "ModelLoader: Loaded static mesh " << filepath
              << " (" << allVertices.size() << " verts, "
              << allIndices.size() / 3 << " tris)" << std::endl;

    return outMesh;
}

// ─── Texture extraction helper ───────────────────────────────────────

static std::unique_ptr<TextureData> ExtractDiffuseTexture(
    const aiScene* scene, const std::string& filepath)
{
    if (scene->mNumMaterials == 0) return nullptr;

    // Look through materials for a diffuse texture
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        const aiMaterial* mat = scene->mMaterials[i];
        if (mat->GetTextureCount(aiTextureType_DIFFUSE) == 0) continue;

        aiString texPath;
        mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
        std::string texPathStr(texPath.C_Str());

        // Check for embedded texture (path starts with '*' or is found via GetEmbeddedTexture)
        const aiTexture* embedded = scene->GetEmbeddedTexture(texPathStr.c_str());
        if (embedded) {
            auto texData = std::make_unique<TextureData>();

            if (embedded->mHeight == 0) {
                // Compressed format (PNG/JPG stored as blob)
                if (texData->LoadFromMemory(
                        reinterpret_cast<const uint8_t*>(embedded->pcData),
                        embedded->mWidth)) {
                    std::cout << "ModelLoader: Extracted embedded texture ("
                              << texData->GetWidth() << "x" << texData->GetHeight()
                              << ")" << std::endl;
                    return texData;
                }
            } else {
                // Raw ARGB8888 pixel data — convert to RGBA
                int w = embedded->mWidth;
                int h = embedded->mHeight;
                std::vector<uint8_t> rgba(w * h * 4);
                for (int p = 0; p < w * h; p++) {
                    const aiTexel& t = embedded->pcData[p];
                    rgba[p * 4 + 0] = t.r;
                    rgba[p * 4 + 1] = t.g;
                    rgba[p * 4 + 2] = t.b;
                    rgba[p * 4 + 3] = t.a;
                }
                texData->Create(w, h, 4, rgba.data());
                std::cout << "ModelLoader: Extracted raw embedded texture ("
                          << w << "x" << h << ")" << std::endl;
                return texData;
            }
        }

        // External file — resolve relative to model directory
        std::filesystem::path modelDir = std::filesystem::path(filepath).parent_path();
        std::filesystem::path fullPath = modelDir / texPathStr;

        auto texData = std::make_unique<TextureData>();
        if (texData->LoadFromFile(fullPath.string())) {
            std::cout << "ModelLoader: Loaded external texture " << fullPath << std::endl;
            return texData;
        }

        std::cerr << "ModelLoader: Failed to load texture " << fullPath << std::endl;
    }

    return nullptr;
}

// ─── Unified LoadModel ──────────────────────────────────────────────

ModelResult ModelLoader::LoadModel(const std::string& filepath) {
    ModelResult result;

    auto t0 = std::chrono::high_resolution_clock::now();
    std::cout << "ModelLoader::LoadModel: Reading " << filepath << " ..." << std::endl;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_LimitBoneWeights |
        aiProcess_FlipUVs
    );

    auto t1 = std::chrono::high_resolution_clock::now();
    auto readMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "ModelLoader::LoadModel: ReadFile completed in " << readMs << " ms" << std::endl;

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ModelLoader::LoadModel: Failed to load: " << filepath
                  << " — " << importer.GetErrorString() << std::endl;
        return result;
    }

    // Auto-detect: check if any mesh has bones
    bool hasBones = false;
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        if (scene->mMeshes[m]->mNumBones > 0) {
            hasBones = true;
            break;
        }
    }

    result.isSkinned = hasBones;

    if (hasBones) {
        // ── Skinned path ──

        // Step 1: Collect bone offset matrices
        std::unordered_map<std::string, aiMatrix4x4> boneOffsetMap;
        for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
            const aiMesh* aiMsh = scene->mMeshes[m];
            for (unsigned int b = 0; b < aiMsh->mNumBones; b++) {
                const aiBone* bone = aiMsh->mBones[b];
                boneOffsetMap[std::string(bone->mName.C_Str())] = bone->mOffsetMatrix;
            }
        }

        // Step 2: Build skeleton
        auto skeleton = std::make_shared<Skeleton>();
        std::unordered_set<std::string> relevantNodes;
        CollectBoneAncestors(scene->mRootNode, boneOffsetMap, relevantNodes);
        BuildSkeletonRecursive(scene->mRootNode, -1, *skeleton, boneOffsetMap, relevantNodes);
        result.skeleton = skeleton;

        std::cout << "ModelLoader::LoadModel: Built skeleton with "
                  << skeleton->GetBoneCount() << " bones" << std::endl;

        // Step 3: Build skinned mesh
        std::vector<SkinnedVertex> allVertices;
        std::vector<uint32_t> allIndices;
        uint32_t vertexOffset = 0;

        for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
            const aiMesh* mesh = scene->mMeshes[m];
            size_t baseVertex = allVertices.size();
            allVertices.resize(baseVertex + mesh->mNumVertices);

            // Read material diffuse color for this sub-mesh
            Math::Vector4 matColor(1, 1, 1, 1);
            if (mesh->mMaterialIndex < scene->mNumMaterials) {
                aiColor4D diffuse;
                if (scene->mMaterials[mesh->mMaterialIndex]->Get(
                        AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS) {
                    matColor = Math::Vector4(diffuse.r, diffuse.g, diffuse.b, diffuse.a);
                }
            }

            for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
                SkinnedVertex& vert = allVertices[baseVertex + v];
                vert.position = ToVec3(mesh->mVertices[v]);
                if (mesh->mNormals) vert.normal = ToVec3(mesh->mNormals[v]);
                if (mesh->mTextureCoords[0]) vert.texCoord = ToVec2(mesh->mTextureCoords[0][v]);
                if (mesh->mTangents) vert.tangent = ToVec3(mesh->mTangents[v]);
                if (mesh->mColors[0]) {
                    vert.color = Math::Vector4(
                        mesh->mColors[0][v].r, mesh->mColors[0][v].g,
                        mesh->mColors[0][v].b, mesh->mColors[0][v].a);
                } else {
                    vert.color = matColor;
                }
            }

            for (unsigned int b = 0; b < mesh->mNumBones; b++) {
                const aiBone* bone = mesh->mBones[b];
                int boneIndex = skeleton->GetBoneIndex(std::string(bone->mName.C_Str()));
                if (boneIndex < 0) continue;
                for (unsigned int w = 0; w < bone->mNumWeights; w++) {
                    uint32_t vertIdx = static_cast<uint32_t>(baseVertex) + bone->mWeights[w].mVertexId;
                    allVertices[vertIdx].AddBoneInfluence(
                        static_cast<uint32_t>(boneIndex), bone->mWeights[w].mWeight);
                }
            }

            for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
                const aiFace& face = mesh->mFaces[f];
                for (unsigned int i = 0; i < face.mNumIndices; i++)
                    allIndices.push_back(vertexOffset + face.mIndices[i]);
            }
            vertexOffset += mesh->mNumVertices;
        }

        for (auto& v : allVertices) v.NormalizeBoneWeights();

        auto* skinnedMesh = new SkinnedMesh(filepath);
        skinnedMesh->SetSkinnedData(allVertices, allIndices);
        result.mesh = skinnedMesh;

        // Step 4: Extract animations
        result.animations = ExtractAnimations(scene, filepath);

        std::cout << "ModelLoader::LoadModel: Skinned mesh loaded ("
                  << allVertices.size() << " verts, "
                  << allIndices.size() / 3 << " tris)" << std::endl;

    } else {
        // ── Static path ──
        std::vector<Vertex> allVertices;
        std::vector<uint32_t> allIndices;
        uint32_t vertexOffset = 0;

        for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
            const aiMesh* mesh = scene->mMeshes[m];

            // Read material diffuse color for this sub-mesh
            Math::Vector4 matColor(1, 1, 1, 1);
            if (mesh->mMaterialIndex < scene->mNumMaterials) {
                aiColor4D diffuse;
                if (scene->mMaterials[mesh->mMaterialIndex]->Get(
                        AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS) {
                    matColor = Math::Vector4(diffuse.r, diffuse.g, diffuse.b, diffuse.a);
                }
            }

            for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
                Vertex vert;
                vert.position = ToVec3(mesh->mVertices[v]);
                if (mesh->mNormals) vert.normal = ToVec3(mesh->mNormals[v]);
                if (mesh->mTextureCoords[0]) vert.texCoord = ToVec2(mesh->mTextureCoords[0][v]);
                if (mesh->mTangents) vert.tangent = ToVec3(mesh->mTangents[v]);
                if (mesh->mColors[0]) {
                    vert.color = Math::Vector4(
                        mesh->mColors[0][v].r, mesh->mColors[0][v].g,
                        mesh->mColors[0][v].b, mesh->mColors[0][v].a);
                } else {
                    vert.color = matColor;
                }
                allVertices.push_back(vert);
            }

            for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
                const aiFace& face = mesh->mFaces[f];
                for (unsigned int i = 0; i < face.mNumIndices; i++)
                    allIndices.push_back(vertexOffset + face.mIndices[i]);
            }
            vertexOffset += mesh->mNumVertices;
        }

        auto* outMesh = new Mesh(filepath);
        outMesh->SetData(allVertices, allIndices);
        result.mesh = outMesh;

        std::cout << "ModelLoader::LoadModel: Static mesh loaded ("
                  << allVertices.size() << " verts, "
                  << allIndices.size() / 3 << " tris)" << std::endl;
    }

    // Extract diffuse texture from materials
    result.diffuseTexture = ExtractDiffuseTexture(scene, filepath);

    return result;
}