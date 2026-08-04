#pragma once
#include <unordered_map>
#include <network/NetTypes.h>
#include <network/trans/SnapshotInterpolator.h>
#include "ServerGameLoop.h"

struct AnimConfig {
    int frameCount;
    float durationPerFrame;  // seconds
    bool loops;
};

// Matches the client's initPlayerAnimations() data
const std::unordered_map<AnimType, AnimConfig> kPlayerAnims = {
    { AnimType::Idle,     { 10, 0.10f, true  } },
    { AnimType::Walk,     {  8, 0.08f, true  } },
    { AnimType::Attack1,  {  7, 0.07f, false } },
    { AnimType::Attack2,  {  6, 0.07f, false } },
    { AnimType::Attack3,  {  9, 0.07f, false } },
    { AnimType::JumpUp,   {  3, 0.10f, false } },
    { AnimType::JumpDown, {  3, 0.10f, false } },
    { AnimType::Hit,      {  3, 0.08f, false } },
    { AnimType::Death,    { 11, 0.12f, false } },
};

bool animLoops(AnimType type) {
    auto it = kPlayerAnims.find(type);
    return it != kPlayerAnims.end() && it->second.loops;
}

// Helper: how many ticks does this animation last?
uint32_t animDurationTicks(AnimType type) {
    auto it = kPlayerAnims.find(type);
    if (it == kPlayerAnims.end()) return 0;
    double TICK_DURATION = 1.0 / 60.0;
    return static_cast<uint32_t>(it->second.frameCount * it->second.durationPerFrame / TICK_DURATION);
}

