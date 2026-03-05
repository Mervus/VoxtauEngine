//
// Created by Marvin on 27/02/2026.
//

#ifndef VOXTAU_PLAYERINPUTSTATE_H
#define VOXTAU_PLAYERINPUTSTATE_H
#include <cstdint>

// What the client sends to the server each tick
struct PlayerInputState {
    uint32_t tick = 0;
    float moveX = 0.0f;
    float moveY = 0.0f;
    float moveZ = 0.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    uint16_t buttons = 0;
};


#endif //VOXTAU_PLAYERINPUTSTATE_H