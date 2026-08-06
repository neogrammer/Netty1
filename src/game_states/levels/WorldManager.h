#pragma once
#include <SFML/Network/Packet.hpp>
#include <SFML/Network/UdpSocket.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <network/NetTypes.h>
#include <game_states/levels/Level.h>
#include <network/server/CombatSystem.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

struct PlayerSlot;

class WorldManager {
public:
    Level level;
    uint32_t nextEntityId = 0;
    std::vector<uint32_t> pendingEntityRemovals;

    static constexpr float SCREEN_W = 1600.0f;
    static constexpr float SCREEN_H = 900.0f;

    void resetWorld(uint32_t& serverTick, uint32_t playerEntityId[2], PlayerSlot (&slots)[2],
        std::unordered_map<uint32_t, CombatantState>& combatants);
    void initializeWorld(uint32_t& nextEntityId, uint32_t serverTick,
        uint32_t playerEntityId[2], PlayerSlot (&slots)[2],
        std::unordered_map<uint32_t, CombatantState>& combatants,
        CombatSystem& combatSystem);
    void manageEntityVisibility(int playerIdx, PlayerSlot (&slots)[2], std::unordered_map<uint32_t, CombatantState>& combatants);
    void buildAndSendSnapshot(int playerIdx, PlayerSlot(&slots)[2], uint32_t serverTick,
        uint32_t playerEntityId[2],
        std::unordered_map<uint32_t, CombatantState>& combatants,
        sf::UdpSocket& udpSocket,
        const std::unordered_map<uint32_t, int>& enemyFacing);
    void processRemovals();

private:
    void sendSpawnToPlayer(int idx, const SpawnMessage& msg, PlayerSlot (&slots)[2]);
    void sendDestroyToPlayer(int idx, const DestroyMessage& msg, PlayerSlot (&slots)[2]);
    void disconnectPlayerCallback(int idx, PlayerSlot (&slots)[2],
        std::unordered_map<uint32_t, CombatantState>& combatants);
};