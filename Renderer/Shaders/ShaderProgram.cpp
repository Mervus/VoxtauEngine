//
// Created by Marvin on 28/01/2026.
//

#include "ShaderProgram.h"

ShaderProgram::ShaderProgram(const std::string& name)
    : name(name)
    , vertexShader(nullptr)
    , pixelShader(nullptr)
    , geometryShader(nullptr)
{
}

ShaderProgram::~ShaderProgram() {
    delete vertexShader;
    delete pixelShader;
    delete geometryShader;
}

void ShaderProgram::SetVertexShader(Shader* vs) {
    vertexShader = vs;
}

void ShaderProgram::SetPixelShader(Shader* ps) {
    pixelShader = ps;
}

void ShaderProgram::SetGeometryShader(Shader* gs) {
    geometryShader = gs;
}

Shader* ShaderProgram::GetVertexShader() const {
    return vertexShader;
}

Shader* ShaderProgram::GetPixelShader() const {
    return pixelShader;
}

Shader* ShaderProgram::GetGeometryShader() const {
    return geometryShader;
}

const std::string& ShaderProgram::GetName() const {
    return name;
}

bool ShaderProgram::IsValid() const {
    // At minimum, need vertex and pixel shader
    return vertexShader != nullptr && pixelShader != nullptr;
}