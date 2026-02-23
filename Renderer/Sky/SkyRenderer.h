//
// Created by Marvin on 12/02/2026.
//

#ifndef VOXTAU_SKYRENDERER_H
#define VOXTAU_SKYRENDERER_H
#pragma once

#include <cstdint>
#include "Core/Math/MathTypes.h"
#include "Renderer/Shaders/ShaderTypes.h"

class Texture;
class IRendererApi;
class ShaderCollection;
class ShaderProgram;
class Mesh;

class SkyRenderer {
public:
    SkyRenderer(IRendererApi* renderer, ShaderCollection* shaderCollection);
    ~SkyRenderer();

    void Initialize();
    void Shutdown();

    // Call each frame with camera info
    void Render(const Math::Matrix4x4& viewProjection,
                const Math::Vector3& cameraPosition,
                float totalTime);

    // Time of day: 0.0 = midnight, 0.25 = sunrise, 0.5 = noon, 0.75 = sunset
    void SetTimeOfDay(float time) { _timeOfDay = time; }
    float GetTimeOfDay() const { return _timeOfDay; }

    // Day/night cycle speed: 0 = frozen, 1.0 = normal
    void SetDaySpeed(float speed) { _daySpeed = speed; }
    bool LoadSkyboxTexture(const std::string& filepath);

    // Get current sun direction (for lighting other systems)
    Math::Vector3 GetSunDirection() const;

private:
    IRendererApi* _renderer;
    ShaderCollection* _shaderCollection;

    Texture* _skyboxTexture = nullptr;
    Mesh* _domeMesh = nullptr;
    void* _skyVertexCB = nullptr;    // b0
    void* _skyPropertiesCB = nullptr; // b1

    float _timeOfDay = 0.35f;  // Start at mid-morning
    float _daySpeed = 0.0f;    // Frozen by default

    void UpdateSunMoonPositions(SkyPropertiesConstants& props);
};


#endif //VOXTAU_SKYRENDERER_H