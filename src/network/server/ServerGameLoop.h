#pragma once
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Network/UdpSocket.hpp>
#include <SFML/Network/TcpListener.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <network/NetTypes.h>
#include <game_states/levels/Level.h>
#include <vector>
#include <optional>
#include <unordered_set>
#include <cstdint>
#include <atomic>
#include <network/server/Combat.h>
#include <SFML/Graphics/Rect.hpp>
struct PlayerSlot {
    sf::TcpSocket tcpSocket;
    unsigned short udpPort = 0;
    std::optional<sf::IpAddress> ip;
    bool connected = false;
    float camX = 0.f;
    bool ready = false;
    bool isJumping = false;
    uint32_t jumpStartTick = 0;
    float jumpStartY = 750.f;
    std::unordered_set<uint32_t> knownEntities;
    uint32_t idleTicks = 0;
    int dir = 0;
    int vertDir = 0;
    int facing = 1;
    bool wantsAttack1 = false;
    bool wantsAttack2 = false;
    bool wantsAttack3 = false;
    bool wantsJump = false;
    // Attack combo tracking
    AnimType currentAttack = AnimType::Idle;  // which attack animation is playing


};



enum class ServerPhase { Lobby, Playing, GameOver };

class ServerGameLoop {
    // Jump timing (in ticks at 60Hz)
    static constexpr uint32_t JUMP_UP_DURATION = 18;   // rising phase (~300ms)
    static constexpr uint32_t JUMP_DOWN_DURATION = 18;   // falling phase (~300ms)
    static constexpr uint32_t JUMP_TOTAL_DURATION = JUMP_UP_DURATION + JUMP_DOWN_DURATION;


public:
    ServerGameLoop(sf::TcpListener& listener,
        sf::TcpSocket tcpSockets[2],
        unsigned short clientPorts[2],
        int initialPlayerCount);

    void run();
    void shutdown();

private:
    // --- Handshake ---
    void waitForUdpHandshake();

    // --- Lifecycle ---
    void resetWorld();
    void disconnectPlayer(int idx);
    void initializeWorld();

    // --- Networking ---
    void acceptLateJoin();
    void processPlayerInput();
    void sendSpawnToPlayer(int idx, const SpawnMessage& msg);
    void sendDestroyToPlayer(int idx, const DestroyMessage& msg);
    void sendAssignPlayerEntity(int idx);

    // --- Game tick ---
    void tickGameLogic();
    void manageEntityVisibility(int playerIdx);
    void buildAndSendSnapshot(int playerIdx);
    bool hitboxesOverlap(const Entity& a, const Entity& b);

    void processAttacks();
    sf::FloatRect getStrikeBox(const Entity& entity, const StrikeBox& sb);
    bool strikeHitsTarget(const sf::FloatRect& strikeBox, const Entity& target,
        const Entity& attacker, float depthTolerance);
    int calculateDamage(const CombatantState& attacker, const CombatantState& defender);
    int attackIndex(AnimType type);

    // --- Members ---
    sf::TcpListener& listener;
    sf::UdpSocket udpSocket;
    PlayerSlot slots[2];
    Level level;
    ServerPhase phase = ServerPhase::Lobby;

    uint32_t nextEntityId = 0;
    uint32_t serverTick = 0;
    uint32_t playerEntityId[2] = { 0xFFFFFFFF, 0xFFFFFFFF };
    int playerCount = 0;


    std::unordered_map<uint32_t, CombatantState> combatants;

    std::atomic<bool> running{ true };

    static constexpr double TICK_DURATION = 1.0 / 60.0;
    static constexpr unsigned short UDP_PORT = 57913;
    static constexpr float SCREEN_W = 1600.0f;
    static constexpr float SCREEN_H = 900.0f;
    static constexpr float DEAD_ZONE = SCREEN_W / 3.0f;
    static constexpr float LEFT_WORLD = 0.0f;
    static constexpr float RIGHT_WORLD = 18000.0f;
    static constexpr float PLAYER_SPEED = 300.0f;
};