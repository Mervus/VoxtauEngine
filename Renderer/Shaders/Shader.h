//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_SHADER_H
#define DIRECTX11_SHADER_H
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "ShaderTypes.h"
#include "EngineApi.h"

class Shader {
protected:
    std::string name;
    std::string filepath;
    ShaderType type;
    void* shaderHandle; // Platform-specific (ID3D11VertexShader*, etc.)
    std::vector<uint8_t> bytecode; // Stored for input layout creation

public:
    Shader(const std::string& name, const std::string& filepath, ShaderType type);
    virtual ~Shader();

    const std::string& GetName() const;
    const std::string& GetFilepath() const;
    ShaderType GetType() const;
    void* GetHandle() const;

    void SetHandle(void* handle);

    // Bytecode access (for input layout creation)
    void SetBytecode(const void* data, size_t size);
    const void* GetBytecode() const;
    size_t GetBytecodeSize() const;
};


#endif //DIRECTX11_SHADER_H