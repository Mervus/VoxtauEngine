//
// Created by Marvin on 12/02/2026.
//

#include "SkyRenderer.h"

#include "Renderer/RenderApi/IRendererApi.h"
#include "Renderer/Shaders/ShaderCollection.h"
#include "Renderer/Shaders/ShaderProgram.h"
#include "Resources/MeshGenerator.h"
#include <cmath>
#include <iostream>

SkyRenderer::SkyRenderer(IRendererApi* renderer, ShaderCollection* shaderCollection)
    : _renderer(renderer)
    , _shaderCollection(shaderCollection)
{
}

SkyRenderer::~SkyRenderer()
{
    Shutdown();
}

void SkyRenderer::Initialize()
{
    // Create inverted sphere (we render from inside)
    // Large radius doesn't matter — vertex shader sets z = w (always at far plane)
    _domeMesh = MeshGenerator::CreateSphere(1.0f, 32, 16);

    // Create GPU buffers for the mesh
    _renderer->CreateMesh(_domeMesh);

    // Create input layout matching the sky vertex shader
    ShaderProgram* skyShader = _shaderCollection->GetSkyShader();
    if (skyShader && skyShader->GetVertexShader()) {
        _renderer->CreateMeshInputLayout(_domeMesh, skyShader->GetVertexShader());
    }

    // Constant buffers
    _skyVertexCB = _renderer->CreateConstantBuffer(sizeof(SkyVertexConstants));
    _skyPropertiesCB = _renderer->CreateConstantBuffer(sizeof(SkyPropertiesConstants));

    std::cout << "SkyRenderer initialized." << std::endl;
}

void SkyRenderer::Shutdown()
{
    if (_domeMesh) {
        delete _domeMesh;
        _domeMesh = nullptr;
    }
    if (_skyVertexCB) {
        _renderer->DestroyConstantBuffer(_skyVertexCB);
        _skyVertexCB = nullptr;
    }
    if (_skyPropertiesCB) {
        _renderer->DestroyConstantBuffer(_skyPropertiesCB);
        _skyPropertiesCB = nullptr;
    }
}

Math::Vector3 SkyRenderer::GetSunDirection() const
{
    // Sun orbits: at timeOfDay=0.5 (noon) sun is at zenith
    // At 0.25 (sunrise) sun is at horizon east, 0.75 (sunset) at horizon west
    float sunAngle = (_timeOfDay - 0.25f) * 2.0f * static_cast<float>(Math::PI);
    return Math::Vector3(
        cosf(sunAngle),
        sinf(sunAngle),
        0.2f  // Slight Z offset so sun isn't perfectly in XY plane
    ).Normalized();
}

void SkyRenderer::UpdateSunMoonPositions(SkyPropertiesConstants& props)
{
    // Sun
    Math::Vector3 sunDir = GetSunDirection();
    props.sunDirection[0] = sunDir.x;
    props.sunDirection[1] = sunDir.y;
    props.sunDirection[2] = sunDir.z;
    props.sunIntensity = 2.0f;
    props.sunColor[0] = 1.0f;
    props.sunColor[1] = 0.95f;
    props.sunColor[2] = 0.8f;
    props.sunSize = 0.04f;

    // Moon: opposite side of sky from sun
    props.moonDirection[0] = -sunDir.x;
    props.moonDirection[1] = -sunDir.y;
    props.moonDirection[2] = -sunDir.z + 0.1f; // Slight offset
    props.moonIntensity = 0.6f;
    props.moonColor[0] = 0.9f;
    props.moonColor[1] = 0.9f;
    props.moonColor[2] = 1.0f;
    props.moonSize = 0.03f;
}

void SkyRenderer::Render(const Math::Matrix4x4& viewProjection,
                          const Math::Vector3& cameraPosition,
                          float totalTime)
{
    if (!_domeMesh) return;

    ShaderProgram* skyShader = _shaderCollection->GetSkyShader();
    if (!skyShader) {
        // Not loaded yet — shouldn't happen if ShaderCollection::Initialize loads it
        std::cerr << "SkyRenderer: Sky shader not loaded!" << std::endl;
        return;
    }

    // Advance time of day
    if (_daySpeed > 0.0f) {
        _timeOfDay += _daySpeed * (1.0f / 60.0f) * (1.0f / 600.0f); // ~10 min per full cycle at speed=1
        if (_timeOfDay > 1.0f) _timeOfDay -= 1.0f;
    }

    // ── Update vertex constants (b0) ──
    SkyVertexConstants vertexConst;
    vertexConst.viewProjection = viewProjection.Transposed();
    vertexConst.cameraPosition[0] = cameraPosition.x;
    vertexConst.cameraPosition[1] = cameraPosition.y;
    vertexConst.cameraPosition[2] = cameraPosition.z;
    vertexConst.time = totalTime;

    _renderer->UpdateConstantBuffer(_skyVertexCB, &vertexConst, sizeof(vertexConst));

    // ── Update sky properties (b1) ──
    SkyPropertiesConstants skyProps = {};

    UpdateSunMoonPositions(skyProps);

    // Sky colors (used as fallback; pixel shader also has hardcoded colors)
    skyProps.zenithColor[0] = 0.1f;
    skyProps.zenithColor[1] = 0.3f;
    skyProps.zenithColor[2] = 0.9f;
    skyProps.timeOfDay = _timeOfDay;

    skyProps.horizonColor[0] = 0.7f;
    skyProps.horizonColor[1] = 0.85f;
    skyProps.horizonColor[2] = 1.0f;

    skyProps.nightZenithColor[0] = 0.01f;
    skyProps.nightZenithColor[1] = 0.01f;
    skyProps.nightZenithColor[2] = 0.05f;

    skyProps.nightHorizonColor[0] = 0.05f;
    skyProps.nightHorizonColor[1] = 0.05f;
    skyProps.nightHorizonColor[2] = 0.12f;

    skyProps.starIntensity = 1.0f;

    skyProps.atmosphereThickness = 1.0f;
    skyProps.rayleighScattering = 0.05f;
    skyProps.mieScattering = 0.02f;

    _renderer->UpdateConstantBuffer(_skyPropertiesCB, &skyProps, sizeof(skyProps));

    // Render state
    // Depth: read but don't write (sky is always behind everything)
    // Rasterizer: render front faces — we're OUTSIDE the sphere looking IN
    //             OR render back faces if we're INSIDE the sphere
    // Since the vertex shader uses position.xyww (z=w=far plane),
    // depth test must be LessEqual so sky passes at z=1.0

    // Depth: sky vertex shader outputs z=w (depth = 1.0 at far plane)
    // Need LessEqual so sky passes depth test against cleared depth buffer (also 1.0)
    // Don't write depth — sky should never occlude anything
    _renderer->SetDepthMode(DepthMode::LessEqual);
    _renderer->SetDepthWriteEnabled(false);
    _renderer->SetRasterizerMode(RasterizerMode::NoCull); // See both sides of dome

    // Bind and draw
    _renderer->BindShader(skyShader);
    _renderer->BindConstantBuffer(0, _skyVertexCB, ShaderStage::Vertex);
    _renderer->BindConstantBuffer(1, _skyPropertiesCB, ShaderStage::Pixel);

    // No textures bound for now — pixel shader will use procedural sky
    // (cloud/star/moon textures on t0-t2 are sampled but will return 0 if unbound,
    //  the shader handles this gracefully with its procedural fallbacks)

    _renderer->Draw(_domeMesh);

    // ── Restore render state ──
    _renderer->SetDepthMode(DepthMode::Less);
    _renderer->SetDepthWriteEnabled(true);
    _renderer->SetRasterizerMode(RasterizerMode::Solid);
}