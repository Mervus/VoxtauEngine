//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_SHADERPROGRAM_H
#define DIRECTX11_SHADERPROGRAM_H
#pragma once
#include <string>
#include <map>
#include "Shader.h"

class ShaderProgram
{
private:
    std::string name;
    Shader* vertexShader;
    Shader* pixelShader;
    Shader* geometryShader; // Optional

    // For storing uniform buffer metadata (optional for later)
    std::map<std::string, int> uniformLocations;

public:
    ShaderProgram(const std::string& name);
    ~ShaderProgram();

    void SetVertexShader(Shader* vs);
    void SetPixelShader(Shader* ps);
    void SetGeometryShader(Shader* gs);

    [[nodiscard]] Shader* GetVertexShader() const;
    [[nodiscard]] Shader* GetPixelShader() const;
    [[nodiscard]] Shader* GetGeometryShader() const;

    const std::string& GetName() const;

    bool IsValid() const;
};


#endif //DIRECTX11_SHADERPROGRAM_H