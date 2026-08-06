#pragma once
#include <SFML/Graphics/Rect.hpp>
#include <network/NetTypes.h>
#include <network/server/Combat.h>
#include <game_states/levels/Level.h>
#include <unordered_map>
#include <cstdint>

struct PlayerSlot;

class CombatSystem {
public:
    CombatantState createPlayerCombatant();
    int attackIndex(AnimType type);
    int calculateDamage(const CombatantState& attacker, const CombatantState& defender);
    sf::FloatRect getStrikeBox(const Entity& entity, const StrikeBox& sb,
        const PlayerSlot(&slots)[2], uint32_t playerEntityId[2],
        const std::unordered_map<uint32_t, int>& enemyFacing);
    bool strikeHitsTarget(const sf::FloatRect& strikeBox, const Entity& target,
        const Entity& attacker, float depthTolerance);
    void processAttacks(PlayerSlot(&slots)[2], uint32_t playerEntityId[2],
        Level& level, uint32_t serverTick,
        std::unordered_map<uint32_t, CombatantState>& combatants,
        const std::unordered_map<uint32_t, int>& enemyFacing);
};