//
// Created by Marvin on 28/01/2026.
//

#include "Shader.h"

Shader::Shader(const std::string& name, const std::string& filepath, ShaderType type)
    : name(name)
    , filepath(filepath)
    , type(type)
    , shaderHandle(nullptr)
{
}

Shader::~Shader() {
    // Platform-specific cleanup handled by renderer
}

const std::string& Shader::GetName() const {
    return name;
}

const std::string& Shader::GetFilepath() const {
    return filepath;
}

ShaderType Shader::GetType() const {
    return type;
}

void* Shader::GetHandle() const {
    return shaderHandle;
}

void Shader::SetHandle(void* handle) {
    shaderHandle = handle;
}

void Shader::SetBytecode(const void* data, size_t size) {
    bytecode.resize(size);
    std::memcpy(bytecode.data(), data, size);
}

const void* Shader::GetBytecode() const {
    return bytecode.empty() ? nullptr : bytecode.data();
}

size_t Shader::GetBytecodeSize() const {
    return bytecode.size();
}