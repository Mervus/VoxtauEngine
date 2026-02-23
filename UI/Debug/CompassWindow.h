//
// Created by Marvin on 05/02/2026.
//

#ifndef VOXTAU_COMPASSWINDOW_H
#define VOXTAU_COMPASSWINDOW_H
#pragma once

#include "Core/Math/Vector3.h"

class CompassWindow {
private:
    bool isOpen;
    Math::Vector3 forward;

public:
    CompassWindow();

    void Update(const Math::Vector3& cameraForward);
    void Draw();

    void SetOpen(bool open) { isOpen = open; }
    bool IsOpen() const { return isOpen; }
};

#endif //VOXTAU_COMPASSWINDOW_H
