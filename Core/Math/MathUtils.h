//
// Created by Marvin on 28/01/2026.
//

#ifndef DIRECTX11_MATHUTILS_H
#define DIRECTX11_MATHUTILS_H

#pragma once
#include "MathTypes.h"
#include <algorithm>

namespace Math {
    // Clamping
    template<typename T>
    inline T Clamp(T value, T minVal, T maxVal) {
        return (std::max)(minVal, (std::min)(value, maxVal));
    }

    inline float Clamp01(float value) {
        return Clamp(value, 0.0f, 1.0f);
    }

    // Interpolation
    inline float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    inline float LerpClamped(float a, float b, float t) {
        return Lerp(a, b, Clamp01(t));
    }

    inline float InverseLerp(float a, float b, float value) {
        return (value - a) / (b - a);
    }

    // Angle utilities
    inline float DegreesToRadians(float degrees) {
        return degrees * DEG_TO_RAD;
    }

    inline float RadiansToDegrees(float radians) {
        return radians * RAD_TO_DEG;
    }

    // Comparison with epsilon
    inline bool Approximately(float a, float b, float epsilon = EPSILON) {
        return std::abs(a - b) < epsilon;
    }

    // Sign function
    template<typename T>
    inline T Sign(T value) {
        return (value > T(0)) ? T(1) : ((value < T(0)) ? T(-1) : T(0));
    }

    // Min/Max
    template<typename T>
    inline T Min(T a, T b) {
        return (a < b) ? a : b;
    }

    template<typename T>
    inline T Max(T a, T b) {
        return (a > b) ? a : b;
    }

    // Absolute value
    template<typename T>
    inline T Abs(T value) {
        return (value < T(0)) ? -value : value;
    }

    // Smooth step (cubic hermite interpolation)
    inline float SmoothStep(float edge0, float edge1, float x) {
        float t = Clamp01((x - edge0) / (edge1 - edge0));
        return t * t * (3.0f - 2.0f * t);
    }

    // Remap value from one range to another
    inline float Remap(float value, float fromMin, float fromMax, float toMin, float toMax) {
        float t = InverseLerp(fromMin, fromMax, value);
        return Lerp(toMin, toMax, t);
    }

    // Power of 2 checks
    inline bool IsPowerOfTwo(int value) {
        return (value > 0) && ((value & (value - 1)) == 0);
    }

    inline int NextPowerOfTwo(int value) {
        value--;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        value++;
        return value;
    }
}

#endif //DIRECTX11_MATHUTILS_H