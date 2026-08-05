#pragma once
#include <SFML/System/Clock.hpp>
#include <network/NetTypes.h>
#include <network/server/Combat.h>
#include <cstdint>

struct PlayerSlot;

class AnimationController {
public:
    static constexpr uint32_t JUMP_UP_DURATION = 18;
    static constexpr uint32_t JUMP_DOWN_DURATION = 18;
    static constexpr uint32_t JUMP_TOTAL_DURATION = JUMP_UP_DURATION + JUMP_DOWN_DURATION;
    static constexpr uint32_t IDLE_GRACE_TICKS = 8;

    // Process animation state for one player entity. Returns the desired animation.
    AnimType updateAnimation(PlayerSlot& slot, Entity& entity, uint32_t serverTick);
};