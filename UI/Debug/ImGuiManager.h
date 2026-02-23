//
// Created by Marvin on 29/01/2026.
//

#ifndef DIRECTX11_IMGUIMANAGER_H
#define DIRECTX11_IMGUIMANAGER_H

#pragma once
#include <d3d11.h>

class ImGuiManager {
private:
    bool initialized;

public:
    ImGuiManager();
    ~ImGuiManager();

    bool Initialize(void* windowHandle, ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    void BeginFrame();
    void EndFrame();
    void Render();

    bool IsInitialized() const { return initialized; }
};


#endif //DIRECTX11_IMGUIMANAGER_H