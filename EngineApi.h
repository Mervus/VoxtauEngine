//
// Created by Marvin on 28/01/2026.
//
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <iostream>
#include <DirectXMath.h>

constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 6.28318530717958647692f;
constexpr float HALF_PI = 1.57079632679489661923f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;
constexpr float EPSILON = 1e-6f;

template<typename Derived>
struct LoggerMixin {
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique_logged(Args&&... args) {
        return std::make_unique<T>(
            static_cast<Derived*>(this)->getLogger(),
            std::forward<Args>(args)...);
    }
};