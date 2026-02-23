//
// Created by Marvin on 29/01/2026.
//

#ifndef DIRECTX11_PERFORMANCEWINDOW_H
#define DIRECTX11_PERFORMANCEWINDOW_H
#pragma once

class PerformanceWindow {
private:
    bool isOpen;
    float frameTimeHistory[100];
    int frameTimeHistoryIndex;

public:
    PerformanceWindow();

    void Update(float deltaTime);
    void Draw();

    void SetOpen(bool open) { isOpen = open; }
    bool IsOpen() const { return isOpen; }
};


#endif //DIRECTX11_PERFORMANCEWINDOW_H