#pragma once
#include <vector>
#include <cstdint>
#include <network/NetTypes.h>

struct StrikeBox {
    float offsetX = 0.f;
    float offsetY = 0.f;
    float width = 0.f;
    float height = 0.f;
};

struct HotData {
    std::vector<int> hotFrames;       // which frames trigger hit checks
    StrikeBox strikeBox;              // where the strike lands
    float depthTolerance = 15.f;      // feet Y must be within this
    int damage = 0;                   // fixed damage (if overrideStrength)
    bool overrideStrength = false;    // if true, use damage instead of stats.strength
};

// Server-side only — not sent over the network directly
struct CombatConfig {
    CombatStats baseStats;
    std::vector<HotData> attacks;
};

struct CombatantState {
    CombatConfig config;        // static: never changes after creation
    CombatStats stats;          // runtime: current HP, etc.
    bool isAlive = true;
    uint32_t lastHitTick = 0;
    uint32_t lastAttackTick = 0;
    uint32_t deathTick = 0;
    int lastProcessedFrame = -1;  // which frame index was last processed
};