//
// Created by Marvin on 14/02/2026.
//

#include "EntityRenderer.h"

#include "Core/Entity/Entity.h"
#include "Core/Entity/EntityManager.h"
#include "Core/Profiler/Profiler.h"
#include "Renderer/RenderStates.h"
#include "Renderer/RenderApi/IRendererApi.h"
#include "Renderer/Shaders/ShaderTypes.h"
#include "Resources/Animation/Animator.h"

EntityRenderer::EntityRenderer(IRendererApi* renderer) :
    _renderer(renderer)
{

}

void EntityRenderer::Initialize()
{
    // Bone matrix buffer: 128 matrices * 64 bytes = 8192 bytes
    _boneBuffer = _renderer->CreateConstantBuffer(sizeof(Math::Matrix4x4) * MAX_BONES);
}

void EntityRenderer::Shutdown()
{
    if (_boneBuffer) {
        _renderer->DestroyConstantBuffer(_boneBuffer);
        _boneBuffer = nullptr;
    }

    _entityQueue.clear();
}

void EntityRenderer::Render(void* _perObjectBuffer, bool _debugCameraMode)
{
    if (_entityQueue.empty()) return;
    PROFILE_SCOPE("Entities");

    if (_debugCameraMode)
        _renderer->SetRasterizerMode(RasterizerMode::NoCull);

    for (const auto& cmd : _entityQueue) {
        _renderer->BindShader(cmd.shader);

        // Per-object transform (b1)
        PerObjectBuffer perObject;
        perObject.world = cmd.worldMatrix.Transposed();
        perObject.worldInverseTranspose = cmd.worldMatrix.Inverted().Transposed();
        _renderer->UpdateConstantBuffer(_perObjectBuffer, &perObject, sizeof(PerObjectBuffer));
        _renderer->BindConstantBuffer(1, _perObjectBuffer, ShaderStage::Vertex);

        // Bone matrices (b2) — only for skinned entities
        if (cmd.animator && cmd.animator->GetBoneMatrixCount() > 0) {
            // Transpose each matrix for HLSL (column-major on GPU)
            Math::Matrix4x4 transposedBones[MAX_BONES];
            int count = cmd.animator->GetBoneMatrixCount();
            const Math::Matrix4x4* bones = cmd.animator->GetBoneMatrices();
            for (int i = 0; i < count; i++)
                transposedBones[i] = bones[i].Transposed();

            _renderer->UpdateConstantBuffer(
                _boneBuffer,
                transposedBones,
                sizeof(Math::Matrix4x4) * count);
            _renderer->BindConstantBuffer(2, _boneBuffer, ShaderStage::Vertex);
        }

        if (cmd.texture)
            _renderer->BindTexture(0, cmd.texture);

        _renderer->Draw(cmd.mesh);
    }

    if (_debugCameraMode)
        _renderer->SetRasterizerMode(RasterizerMode::Solid);

    // Clear entity queue for next frame
    _entityQueue.clear();
}

void EntityRenderer::SubmitEntity(const EntityRenderCommand& cmd)
{
    _entityQueue.push_back(cmd);
}

void EntityRenderer::SubmitEntities(EntityManager* entityManager)
{
    if (!entityManager) return;

    entityManager->ForEach([this](Entity* entity) {
        RenderData rd = entity->GetRenderData();
        if (!rd.IsValid() || !rd.shader) return;

        EntityRenderCommand cmd;
        cmd.worldMatrix = rd.worldMatrix;
        cmd.mesh = rd.mesh;
        cmd.shader = rd.shader;
        cmd.texture = rd.texture;
        cmd.animator = rd.animator;
        _entityQueue.push_back(cmd);
    });
}

