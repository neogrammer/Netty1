#pragma once
#include <SFML/Network/TcpListener.hpp>
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Network/UdpSocket.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <network/server/CombatSystem.h>
#include <network/NetTypes.h>
#include <cstdint>
#include <optional>
#include <unordered_map>

#include <network/server/ServerPhase.h>


struct PlayerSlot;
class WorldManager;
class CombatSystem;
class ServerGameLoop;

class NetworkManager {
public:
    static constexpr unsigned short UDP_PORT = 57913;

    sf::UdpSocket udpSocket;
    sf::TcpListener& listener;

    NetworkManager(sf::TcpListener& lst);
    void processPlayerInput(PlayerSlot (&slots)[2], WorldManager& world,
        CombatSystem& combat, uint32_t& nextEntityId,
        uint32_t serverTick, uint32_t playerEntityId[2], int& playerCount,
        ServerPhase& phase,
        std::unordered_map<uint32_t, CombatantState>& combatants);
    void waitForUdpHandshake(PlayerSlot (&slots)[2], int& playerCount,
        ServerPhase& phase, WorldManager& world,
        std::unordered_map<uint32_t, CombatantState>& combatants,
        CombatSystem& combat, uint32_t playerEntityId[2]);
    void acceptLateJoin(PlayerSlot (&slots)[2], int& playerCount,
        ServerPhase& phase, WorldManager& world,
        CombatSystem& combat, uint32_t& nextEntityId,
        uint32_t serverTick, uint32_t playerEntityId[2],
        std::unordered_map<uint32_t, CombatantState>& combatants);
    void broadcastPlayerStatus(int idx, PlayerSlot (&slots)[2], uint32_t playerEntityId[2],
        std::unordered_map<uint32_t, CombatantState>& combatants);
    void broadcastAllPlayerStatusTo(int toIdx, PlayerSlot (&slots)[2], uint32_t playerEntityId[2],
        std::unordered_map<uint32_t, CombatantState>& combatants);
    void sendAssignPlayerEntity(int idx, PlayerSlot (&slots)[2], uint32_t playerEntityId[2],
        int& playerCount, ServerPhase& phase, WorldManager& world,
        std::unordered_map<uint32_t, CombatantState>& combatants);

    void disconnectPlayer(int idx, PlayerSlot (&slots)[2], int& playerCount,
        ServerPhase& phase, WorldManager& world,
        std::unordered_map<uint32_t, CombatantState>& combatants,
        uint32_t playerEntityId[2]);
};