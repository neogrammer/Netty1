#pragma once
#include <SFML/Network.hpp>
#include <memory>
#include <vector>
#include <network/NetTypes.h>  
class GameStateManager;

struct ClientContext {
    sf::UdpSocket* udpSocket = nullptr;
    sf::TcpSocket* tcpSocket = nullptr;
    sf::IpAddress serverIp;
    unsigned short serverPort = 0;
    int playerId = 0;
    uint32_t myEntityId = 0xFFFFFFFF;
    int currentLevel = -1;
    int currentZone = -1;

    bool player1Connected = false;
    bool player2Connected = false;
    int player1Health = 100;
    int player1MaxHealth = 100;
    int player2Health = 100;
    int player2MaxHealth = 100;

    GameStateManager* gsm = nullptr;

    // Snapshot queue
    sf::Packet latestSnapshot;
    bool hasSnapshot = false;

    // Spawn/despawn queues
    std::vector<SpawnMessage> pendingSpawns;
    std::vector<DestroyMessage> pendingDestroys;

    // Input for client-side prediction
    std::string latestInput;
};