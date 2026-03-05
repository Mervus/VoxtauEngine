//
// Created by Marvin on 27/02/2026.
//

#ifndef VOXTAU_INPUTBUTTON_H
#define VOXTAU_INPUTBUTTON_H
#pragma once
#include <cstdint>

namespace InputButton {
    constexpr uint16_t Jump      = 1 << 0;
    constexpr uint16_t Attack    = 1 << 1;
    constexpr uint16_t Use       = 1 << 2;
    constexpr uint16_t Sprint    = 1 << 3;
    constexpr uint16_t Crouch    = 1 << 4;
    constexpr uint16_t Block     = 1 << 5;
};
#endif //VOXTAU_INPUTBUTTON_H