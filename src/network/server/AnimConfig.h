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
    { AnimType::Attack1,  {  7, 0.14f, false } },
    { AnimType::Attack2,  {  6, 0.14f, false } },
    { AnimType::Attack3,  {  9, 0.14f, false } },
    { AnimType::JumpUp,   {  3, 0.10f, false } },
    { AnimType::JumpDown, {  3, 0.10f, false } },
    { AnimType::Hit,      {  3, 0.08f, false } },
    { AnimType::Death,    { 11, 0.12f, false } },
};

extern bool animLoops(AnimType type);

// Helper: how many ticks does this animation last?
extern uint32_t animDurationTicks(AnimType type);

const std::unordered_map<AnimType, AnimConfig> kGoblinAnims = {
    { AnimType::Idle,     { 8, 0.12f, true  } },
    { AnimType::Walk,     { 8, 0.08f, true  } },
    { AnimType::Attack1,  { 8, 0.10f, false } },
    { AnimType::Attack2,  { 8, 0.10f, false } },
    { AnimType::Attack3,  { 8, 0.10f, false } },
    { AnimType::JumpUp,   { 1, 0.10f, false } },
    { AnimType::JumpDown, { 1, 0.10f, false } },
    { AnimType::Hit,      { 8, 0.08f, false } },
    { AnimType::Death,    { 8, 0.15f, false } },
};

extern const std::unordered_map<AnimType, AnimConfig>& getAnimTable(EntityType type);