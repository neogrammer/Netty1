#pragma once
#include <SFML/Graphics/Rect.hpp>
#include <network/NetTypes.h>
#include "Combat.h"
#include <unordered_map>
#include <cstdint>

struct PlayerSlot;
class Level;

class CombatSystem {
public:
    // Create a default player combatant
    CombatantState createPlayerCombatant();

    // Process attacks for all players
    void processAttacks(PlayerSlot (&slots)[2], uint32_t playerEntityId[2],
        Level& level, uint32_t serverTick,
        std::unordered_map<uint32_t, CombatantState>& combatants);

    // Damage calculation
    int calculateDamage(const CombatantState& attacker, const CombatantState& defender);

private:
    int attackIndex(AnimType type);
    sf::FloatRect getStrikeBox(const Entity& entity, const StrikeBox& sb,
        const PlayerSlot (&slots)[2], uint32_t playerEntityId[2]);
    bool strikeHitsTarget(const sf::FloatRect& strikeBox, const Entity& target,
        const Entity& attacker, float depthTolerance);
};