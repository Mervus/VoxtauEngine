//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_MATHTYPES_H
#define DIRECTX11_MATHTYPES_H

#pragma once

namespace Math {
    // Constants
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 6.28318530717958647692f;
    constexpr float HALF_PI = 1.57079632679489661923f;
    constexpr float EPSILON = 0.000001f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;

    // Forward declarations
    class Vector2;
    class Vector3;
    class Vector4;
    class Matrix4x4;
    class Quaternion;
}

#endif //DIRECTX11_MATHTYPES_H