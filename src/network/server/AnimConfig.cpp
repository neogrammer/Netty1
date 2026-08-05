#include "AnimConfig.h"


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