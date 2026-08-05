#pragma once
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <network/NetTypes.h>
#include <unordered_set>
#include <optional>
#include <cstdint>

struct PlayerSlot {
    sf::TcpSocket tcpSocket;
    unsigned short udpPort = 0;
    std::optional<sf::IpAddress> ip;
    bool connected = false;
    float camX = 0.f;
    bool ready = false;
    bool readyInPlayState = false;
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
    AnimType currentAttack = AnimType::Idle;
};

class PlayerManager {
public:
    PlayerSlot slots[2];
    uint32_t playerEntityId[2] = { 0xFFFFFFFF, 0xFFFFFFFF };
    int playerCount = 0;

    int getFreeSlot() const;
    bool isConnected(int idx) const;
    uint32_t getEntityId(int idx) const;
};