//
// Created by Marvin on 28/01/2026.
//
#include "DirectX11Mesh.h"
#include <iostream>

DirectX11Mesh::DirectX11Mesh()
    : vertexCount(0)
    , indexCount(0)
    , stride(0)
    , topology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
{
}

DirectX11Mesh::~DirectX11Mesh() {
    // ComPtr automatically releases
}

bool DirectX11Mesh::CreateBuffers(
    ID3D11Device* device,
    const void* vertexData,
    UINT vertexCount,
    UINT vertexStride,
    const uint32_t* indexData,
    UINT indexCount)
{
    // Safety check - can't create buffers with 0 vertices
    if (vertexCount == 0 || vertexData == nullptr) {
        return false;
    }

    this->vertexCount = vertexCount;
    this->indexCount = indexCount;
    this->stride = vertexStride;

    // CREATE VERTEX BUFFER
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.ByteWidth = vertexStride * vertexCount;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = 0;
    vbDesc.MiscFlags = 0;
    vbDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertexData;
    vbData.SysMemPitch = 0;
    vbData.SysMemSlicePitch = 0;

    HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, &vertexBuffer);
    if (FAILED(hr)) {
        std::cerr << "Failed to create vertex buffer! HRESULT: 0x" << std::hex << hr << std::dec
                  << " vertices: " << vertexCount << " stride: " << vertexStride
                  << " bytes: " << (vertexStride * vertexCount) << std::endl;
        return false;
    }

    // CREATE INDEX BUFFER (optional - some meshes use non-indexed rendering)
    if (indexCount > 0 && indexData != nullptr) {
        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.Usage = D3D11_USAGE_DEFAULT;
        ibDesc.ByteWidth = sizeof(uint32_t) * indexCount;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibDesc.CPUAccessFlags = 0;
        ibDesc.MiscFlags = 0;
        ibDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA ibData = {};
        ibData.pSysMem = indexData;
        ibData.SysMemPitch = 0;
        ibData.SysMemSlicePitch = 0;

        hr = device->CreateBuffer(&ibDesc, &ibData, &indexBuffer);
        if (FAILED(hr)) {
            std::cerr << "Failed to create index buffer!" << std::endl;
            return false;
        }
    }

    return true;
}

bool DirectX11Mesh::UpdateVertexBuffer(
    ID3D11DeviceContext* context,
    const void* vertexData,
    UINT vertexCount)
{
    if (!vertexBuffer) {
        std::cerr << "Vertex buffer not created yet!" << std::endl;
        return false;
    }

    // If vertex count changed, need to recreate buffer
    // For now, just update existing data (assumes same size)
    this->vertexCount = vertexCount;

    context->UpdateSubresource(vertexBuffer.Get(), 0, nullptr, vertexData, 0, 0);

    return true;
}

bool DirectX11Mesh::UpdateIndexBuffer(
    ID3D11DeviceContext* context,
    const uint32_t* indexData,
    UINT indexCount)
{
    if (!indexBuffer) {
        std::cerr << "Index buffer not created yet!" << std::endl;
        return false;
    }

    this->indexCount = indexCount;

    context->UpdateSubresource(indexBuffer.Get(), 0, nullptr, indexData, 0, 0);

    return true;
}

bool DirectX11Mesh::CreateInputLayout(
    ID3D11Device* device,
    const void* vertexShaderBytecode,
    SIZE_T bytecodeLength)
{
    // Skip if already created
    if (inputLayout) {
        return true;
    }

    // Define input layout (matches Vertex struct)
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "LIGHT",      0, DXGI_FORMAT_R32_FLOAT,       0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXINDEX",   0, DXGI_FORMAT_R32_UINT,        0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "UVROTATION", 0, DXGI_FORMAT_R32_UINT,        0, 68, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    UINT numElements = ARRAYSIZE(layout);

    HRESULT hr = device->CreateInputLayout(
        layout,
        numElements,
        vertexShaderBytecode,
        bytecodeLength,
        &inputLayout
    );

    if (FAILED(hr)) {
        std::cerr << "Failed to create input layout!" << std::endl;
        return false;
    }

    return true;
}

bool DirectX11Mesh::CreateVoxelInputLayout(ID3D11Device* device, const void* vertexShaderBytecode,
    SIZE_T bytecodeLength)
{
    // Skip if already created
    if (inputLayout) {
        return true;
    }

    // Define input layout (matches Vertex struct)
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "PACKED",   0, DXGI_FORMAT_R32_UINT,        0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 },

    };

    UINT numElements = ARRAYSIZE(layout);

    HRESULT hr = device->CreateInputLayout(
        layout,
        numElements,
        vertexShaderBytecode,
        bytecodeLength,
        &inputLayout
    );

    if (FAILED(hr)) {
        std::cerr << "Failed to create voxel vertex input layout!" << std::endl;
        return false;
    }

    return true;
}

bool DirectX11Mesh::CreateInputLayoutFromDesc(ID3D11Device* device, const D3D11_INPUT_ELEMENT_DESC* layoutDesc,
    UINT numElements, const void* vertexShaderBytecode, SIZE_T bytecodeLength)
{
    if (inputLayout) {
        return true;
    }

    HRESULT hr = device->CreateInputLayout(
        layoutDesc,
        numElements,
        vertexShaderBytecode,
        bytecodeLength,
        &inputLayout
    );

    if (FAILED(hr)) {
        std::cerr << "Failed to create input layout! HRESULT: 0x"
                  << std::hex << hr << std::dec << std::endl;
        return false;
    }

    return true;
}

void DirectX11Mesh::Bind(ID3D11DeviceContext* context) {
    // Bind vertex buffer
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);

    // Bind index buffer
    context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    // Set input layout
    if (inputLayout) {
        context->IASetInputLayout(inputLayout.Get());
    }

    // Set primitive topology
    context->IASetPrimitiveTopology(topology);
}

ID3D11Buffer* DirectX11Mesh::GetVertexBuffer() const {
    return vertexBuffer.Get();
}

ID3D11Buffer* DirectX11Mesh::GetIndexBuffer() const {
    return indexBuffer.Get();
}

ID3D11InputLayout* DirectX11Mesh::GetInputLayout() const {
    return inputLayout.Get();
}

UINT DirectX11Mesh::GetVertexCount() const {
    return vertexCount;
}

UINT DirectX11Mesh::GetIndexCount() const {
    return indexCount;
}

UINT DirectX11Mesh::GetStride() const {
    return stride;
}