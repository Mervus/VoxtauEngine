//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_DIRECTX11MESH_H
#define DIRECTX11_DIRECTX11MESH_H

#pragma once
#include <d3d11.h>
#include <wrl/client.h>

#include "../../Vertex.h"

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class DirectX11Mesh {
private:
    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;
    ComPtr<ID3D11InputLayout> inputLayout;

    UINT vertexCount;
    UINT indexCount;
    UINT stride;
    D3D11_PRIMITIVE_TOPOLOGY topology;

public:
    DirectX11Mesh();
    ~DirectX11Mesh();

    // Create mesh buffers from vertex/index data
    bool CreateBuffers(
        ID3D11Device* device,
        const void* vertexData,
        UINT vertexCount,
        UINT vertexStride,
        const uint32_t* indexData,
        UINT indexCount
    );

    // Update buffers (for dynamic meshes like voxel chunks)
    bool UpdateVertexBuffer(
        ID3D11DeviceContext* context,
        const void* vertexData,
        UINT vertexCount
    );

    bool UpdateIndexBuffer(
        ID3D11DeviceContext* context,
        const uint32_t* indexData,
        UINT indexCount
    );

    // Create input layout for vertex format
    bool CreateInputLayout(
        ID3D11Device* device,
        const void* vertexShaderBytecode,
        SIZE_T bytecodeLength
    );
    // Create input layout for Voxel vertex format

    bool CreateVoxelInputLayout(
     ID3D11Device* device,
     const void* vertexShaderBytecode,
     SIZE_T bytecodeLength
    );

    bool CreateInputLayoutFromDesc(
     ID3D11Device* device,
     const D3D11_INPUT_ELEMENT_DESC* layoutDesc,
     UINT numElements,
     const void* vertexShaderBytecode,
     SIZE_T bytecodeLength
    );

    // Bind mesh for rendering
    void Bind(ID3D11DeviceContext* context);

    // Getters
    ID3D11Buffer* GetVertexBuffer() const;
    ID3D11Buffer* GetIndexBuffer() const;
    ID3D11InputLayout* GetInputLayout() const;

    UINT GetVertexCount() const;
    UINT GetIndexCount() const;
    UINT GetStride() const;

    // Topology
    void SetTopology(D3D11_PRIMITIVE_TOPOLOGY topo) { topology = topo; }
    D3D11_PRIMITIVE_TOPOLOGY GetTopology() const { return topology; }
};

#endif //DIRECTX11_DIRECTX11MESH_H