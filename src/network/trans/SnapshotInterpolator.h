#pragma once
#include <network/NetTypes.h>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <unordered_map>

struct InterpolatedEntity {
    float x, y;
    uint32_t animStartTick;
};

class SnapshotInterpolator {
public:
    static constexpr float TICK_DURATION = 1.0f / 60.0f;

    // Call when a new snapshot arrives from the server.
    // Pass the deserialized FrameSnapshot (not the raw packet).
    void pushSnapshot(const FrameSnapshot& snap);

    // Call once per frame. Returns true if interpolation is active.
    bool update();

    // Get the interpolated camera position for this frame.
    sf::Vector2f getCameraPosition() const;

    // Get the interpolated position for a specific entity.
    // Returns false if the entity isn't in the current snapshot.
    bool getEntityPosition(uint32_t entityId, float& outX, float& outY) const;

    // Get the continuous render tick (for animation timing).
    float getRenderTick() const { return currentRenderTick; }

    // Direct access to current snapshot entities (for animation flags, etc.)
    const std::vector<EntitySnapshot>& getCurrentEntities() const { return curr.entities; }

    // Find a specific entity in the current snapshot (for flags, facing, etc.)
    const EntitySnapshot* findEntity(uint32_t entityId) const;
    void reset();
private:
    FrameSnapshot prev;
    FrameSnapshot curr;
    sf::Time lastSnapTime;
    bool hasPrev = false;
    float interpT = 0.0f;
    float currentRenderTick = 0.0f;

    sf::Clock clock;
};