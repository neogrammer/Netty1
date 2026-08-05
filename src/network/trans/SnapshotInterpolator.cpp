#include "SnapshotInterpolator.h"
#include <algorithm>
#include <SFML/System/Vector2.hpp>

void SnapshotInterpolator::pushSnapshot(const FrameSnapshot& snap) {
    prev = std::move(curr);
    curr = snap;
    lastSnapTime = clock.getElapsedTime();
    hasPrev = true;
}

bool SnapshotInterpolator::update() {
    if (!hasPrev) return false;

    sf::Time now = clock.getElapsedTime();
    float rawT = (now - lastSnapTime).asSeconds() / TICK_DURATION;
    interpT = std::clamp(rawT, 0.0f, 1.0f);

    currentRenderTick = prev.frameNumber
        + interpT * static_cast<float>(curr.frameNumber - prev.frameNumber);

    return true;
}

sf::Vector2f SnapshotInterpolator::getCameraPosition() const {
    if (!hasPrev)
        return { 100.0f, 450.0f };   // sensible default

    float camX = unquantise(curr.camX_quant);
    float camY = unquantise(curr.camY_quant);
    float camXPrev = unquantise(prev.camX_quant);
    float camYPrev = unquantise(prev.camY_quant);

    return {
        camXPrev + interpT * (camX - camXPrev),
        450.0f   // fixed vertical center for side-scroller
    };
}

bool SnapshotInterpolator::getEntityPosition(uint32_t entityId,
    float& outX, float& outY) const
{
    // Find in current snapshot
    auto currIt = std::find_if(curr.entities.begin(), curr.entities.end(),
        [entityId](const EntitySnapshot& s) { return s.entityId == entityId; });
    if (currIt == curr.entities.end()) return false;

    float x = unquantise(currIt->x_quant);
    float y = unquantise(currIt->y_quant);

    if (hasPrev) {
        auto prevIt = std::find_if(prev.entities.begin(), prev.entities.end(),
            [entityId](const EntitySnapshot& s) { return s.entityId == entityId; });
        if (prevIt != prev.entities.end()) {
            float prevX = unquantise(prevIt->x_quant);
            float prevY = unquantise(prevIt->y_quant);
            x = prevX + interpT * (x - prevX);
            y = prevY + interpT * (y - prevY);
        }
    }

    outX = x;
    outY = y;
    return true;
}

const EntitySnapshot* SnapshotInterpolator::findEntity(uint32_t entityId) const {
    auto it = std::find_if(curr.entities.begin(), curr.entities.end(),
        [entityId](const EntitySnapshot& s) { return s.entityId == entityId; });
    return (it != curr.entities.end()) ? &(*it) : nullptr;
}

void SnapshotInterpolator::reset() {
    hasPrev = false;
    prev = FrameSnapshot{};
    curr = FrameSnapshot{};
    interpT = 0.0f;
    currentRenderTick = 0.0f;
    lastSnapTime = sf::Time::Zero;
    clock.restart();
}