#include "NetworkManager.h"
#include <network/server/ServerGameLoop.h>
#include <game_states/levels/WorldManager.h>
#include <network/server/CombatSystem.h>
#include <network/server/Combat.h>
#include <cstdio>
#include <cstring>
#include <SFML/System/Sleep.hpp>

NetworkManager::NetworkManager(sf::TcpListener& lst) : listener(lst) {
    if (udpSocket.bind(UDP_PORT) != sf::Socket::Status::Done) {
        fprintf(stderr, "[Server] UDP bind failed on port %d\n", UDP_PORT);
        return;
    }
    udpSocket.setBlocking(false);
}

void NetworkManager::waitForUdpHandshake(PlayerSlot (&slots)[2], int& playerCount,
    ServerPhase& phase, WorldManager& world,
    std::unordered_map<uint32_t, CombatantState>& combatants,
    CombatSystem& combat, uint32_t playerEntityId[2]) {



    printf("[Server] Waiting for UDP 'OK' from connected players...\n");

    while (!slots[0].ip.has_value() && !slots[1].ip.has_value()) {
        char buf[64];
        std::size_t received;
        std::optional<sf::IpAddress> senderIp;
        unsigned short senderPort;

        while (udpSocket.receive(buf, sizeof(buf) - 1, received, senderIp, senderPort)
            == sf::Socket::Status::Done) {
            buf[received] = '\0';
            if (std::strcmp(buf, "OK") == 0) {
                for (int i = 0; i < 2; ++i) {
                    if (slots[i].connected && senderPort == slots[i].udpPort
                        && !slots[i].ip.has_value()) {
                        slots[i].ip = senderIp;
                        printf("[Server] Learned endpoint for player %d: %s:%d\n",
                            i, senderIp->toString().c_str(), senderPort);

                        sf::Packet confirm;
                        std::string ok = "OK";
                        confirm << ok;
                        if (slots[i].tcpSocket.send(confirm) != sf::Socket::Status::Done) {
                      

                            disconnectPlayer(i, slots, playerCount, phase, world, combatants, playerEntityId);
                        }
                        break;
                    }
                }
            }
        }

        if (playerCount < 2) {
            sf::TcpSocket newSocket;
            if (listener.accept(newSocket) == sf::Socket::Status::Done) {
                int freeSlot = -1;
                for (int i = 0; i < 2; ++i) {
                    if (!slots[i].connected) { freeSlot = i; break; }
                }
                if (freeSlot >= 0) {
                    unsigned short newUdpPort;
                    sf::Packet pkt;
                    pkt << freeSlot << static_cast<unsigned short>(UDP_PORT);
                    if (newSocket.send(pkt) != sf::Socket::Status::Done) continue;
                    pkt.clear();
                    if (newSocket.receive(pkt) != sf::Socket::Status::Done) continue;
                    pkt >> newUdpPort;

                    slots[freeSlot].tcpSocket = std::move(newSocket);
                    slots[freeSlot].tcpSocket.setBlocking(false);
                    slots[freeSlot].udpPort = newUdpPort;
                    slots[freeSlot].connected = true;
                    playerCount++;
                    printf("[Server] Late player %d registered during handshake (UDP %d)\n",
                        freeSlot, newUdpPort);

                    char okBuf[4]; std::size_t recvd;
                    std::optional<sf::IpAddress> ip; unsigned short port;
                    while (udpSocket.receive(okBuf, 3, recvd, ip, port)
                        != sf::Socket::Status::Done
                        || std::strncmp(okBuf, "OK", 2) != 0
                        || port != newUdpPort) {
                        sf::sleep(sf::milliseconds(10));
                    }
                    slots[freeSlot].ip = ip;
                    printf("[Server] Learned endpoint for late player %d: %s:%d\n",
                        freeSlot, ip->toString().c_str(), port);

                    sf::Packet confirm;
                    std::string okStr = "OK";
                    confirm << okStr;
                    if (slots[freeSlot].tcpSocket.send(confirm) != sf::Socket::Status::Done) {
						
                        disconnectPlayer(freeSlot, slots, playerCount, phase, world,   combatants,  playerEntityId);
                    }
                        //if (playerCount == 1 && phase == ServerPhase::Lobby) {
                        //    
                        //    world.initializeWorld(world.nextEntityId, 0, playerEntityId, slots, combatants, combat);
                        //    phase = ServerPhase::Playing;
                        //}
                }
            }
        }

        if (slots[0].ip.has_value() || slots[1].ip.has_value())
            break;

        sf::sleep(sf::milliseconds(10));
    }
}

void NetworkManager::acceptLateJoin(PlayerSlot (&slots)[2], int& playerCount,
    ServerPhase& phase, WorldManager& world,
    CombatSystem& combat, uint32_t& nextEntityId,
    uint32_t serverTick, uint32_t playerEntityId[2],
    std::unordered_map<uint32_t, CombatantState>& combatants) {
    sf::TcpSocket newSocket;
    if (listener.accept(newSocket) != sf::Socket::Status::Done) return;

    int freeSlot = -1;
    for (int i = 0; i < 2; ++i) {
        if (!slots[i].connected) { freeSlot = i; break; }
    }
    if (freeSlot < 0) return;

    unsigned short newUdpPort;
    sf::Packet pkt;
    pkt << freeSlot << static_cast<unsigned short>(UDP_PORT);
    if (newSocket.send(pkt) != sf::Socket::Status::Done) return;
    pkt.clear();
    if (newSocket.receive(pkt) != sf::Socket::Status::Done) return;
    pkt >> newUdpPort;

    slots[freeSlot].tcpSocket = std::move(newSocket);
    slots[freeSlot].tcpSocket.setBlocking(false);
    slots[freeSlot].udpPort = newUdpPort;
    slots[freeSlot].connected = true;
    playerCount++;
    printf("[Server] Late player %d joined (UDP %d)\n", freeSlot, newUdpPort);

    char okBuf[4]; std::size_t recvd;
    std::optional<sf::IpAddress> ip; unsigned short port;
    while (udpSocket.receive(okBuf, 3, recvd, ip, port) != sf::Socket::Status::Done
        || std::strncmp(okBuf, "OK", 2) != 0
        || port != newUdpPort) {
        sf::sleep(sf::milliseconds(10));
    }
    slots[freeSlot].ip = ip;
    printf("[Server] Learned endpoint for late player %d: %s:%d\n",
        freeSlot, ip->toString().c_str(), port);

    sf::Packet confirm;
    std::string okStr = "OK";
    confirm << okStr;
    if (slots[freeSlot].tcpSocket.send(confirm) != sf::Socket::Status::Done) {
       
        disconnectPlayer(freeSlot, slots, playerCount, phase, world,
            combatants, playerEntityId);
        return;
    }

    //if (playerCount == 1 && phase == ServerPhase::Lobby) {
    //    
    //    world.initializeWorld(nextEntityId, serverTick, playerEntityId, slots,
    //        combatants, combat);
    //    phase = ServerPhase::Playing;
    //}
    

    broadcastAllPlayerStatusTo(freeSlot, slots, playerEntityId,
        combatants);

    printf("[Server] Late player %d fully initialized (waiting for READY)\n", freeSlot);
}

void NetworkManager::processPlayerInput(PlayerSlot (&slots)[2], WorldManager& world,
    CombatSystem& combat, uint32_t& nextEntityId,
    uint32_t serverTick, uint32_t playerEntityId[2], int& playerCount,
    ServerPhase& phase,
    std::unordered_map<uint32_t, CombatantState>& combatants)
{
    char buf[64];
    std::size_t received;
    std::optional<sf::IpAddress> senderIp;
    unsigned short senderPort;

    bool receivedThisTick[2] = { false, false };

    while (udpSocket.receive(buf, sizeof(buf) - 1, received, senderIp, senderPort)
        == sf::Socket::Status::Done) {
        buf[received] = '\0';

        int playerIdx = (senderPort == slots[0].udpPort) ? 0
            : (senderPort == slots[1].udpPort) ? 1 : -1;
        if (playerIdx < 0 || !slots[playerIdx].connected) continue;

        receivedThisTick[playerIdx] = true;

        if (std::strcmp(buf, "READY") == 0) {
            if (!slots[playerIdx].ready) {
                slots[playerIdx].ready = true;

                if (playerEntityId[playerIdx] == 0xFFFFFFFF) {
                    Entity e;
                    e.id = playerIdx;
                    e.type = EntityType::Player;
                    e.x = (playerIdx == 0) ? 100.f : 700.f;
                    e.y = 750.f;
                    e.animation = static_cast<uint8_t>(AnimType::Idle);
                    e.animStartTick = serverTick;
                    e.hitbox = { 96.f, 84.f, 74.f, 80.f };
                    world.level.addEntity(e);
                    playerEntityId[playerIdx] = e.id;
                    // combatants handled by ServerGameLoop
                    slots[playerIdx].knownEntities.insert(e.id);
                    combatants[e.id] = combat.createPlayerCombatant();
                    slots[playerIdx].camX = e.x;

                    sendAssignPlayerEntity(playerIdx, slots, playerEntityId, playerCount, phase, world, combatants);

                    for (auto& ent : world.level.allEntities) {
                        SpawnMessage spawnMsg{ ent.id, ent.type, ent.x, ent.y,
                                               ent.animation, ent.animStartTick };
                        sf::Packet sp;
                        sp << NetMsgType::SpawnEntity << spawnMsg;
                        slots[playerIdx].tcpSocket.send(sp);
                    }

                    SpawnMessage newPlayerMsg{ e.id, e.type, e.x, e.y,
                                               e.animation, e.animStartTick, 100, 100 };
                    for (int i = 0; i < 2; ++i) {
                        if (i != playerIdx && slots[i].connected) {
                            sf::Packet sp;
                            sp << NetMsgType::SpawnEntity << newPlayerMsg;
                            slots[i].tcpSocket.send(sp);
                            slots[i].knownEntities.insert(e.id);
                        }
                    }

                    for (int i = 0; i < 2; ++i) {
                        if (slots[i].connected && playerEntityId[i] != 0xFFFFFFFF) {
                            slots[playerIdx].knownEntities.insert(playerEntityId[i]);
                            if (i != playerIdx) {
                                slots[i].knownEntities.insert(playerEntityId[playerIdx]);
                            }
                        }
                    }
                }

                printf("[Server] Player %d is ready – sending LoadLevel\n", playerIdx);
                LoadLevelMessage levelMsg{ 1 };
                sf::Packet p;
                p << NetMsgType::LoadLevel << levelMsg;
                if (slots[playerIdx].tcpSocket.send(p) != sf::Socket::Status::Done) {
					
                    disconnectPlayer(playerIdx, slots, playerCount,
                        phase, world,
                        combatants,
                        playerEntityId);
                }
            }
            continue;
        }

        slots[playerIdx].dir = 0;
        slots[playerIdx].vertDir = 0;
        slots[playerIdx].wantsAttack1 = false;
        slots[playerIdx].wantsAttack2 = false;
        slots[playerIdx].wantsAttack3 = false;
        slots[playerIdx].wantsJump = false;

        if (std::strchr(buf, 'L')) {
            slots[playerIdx].dir = -1;
            printf("[Server] Player %d dir=L\n", playerIdx);
        }
        if (std::strchr(buf, 'R')) {
            slots[playerIdx].dir = 1;
            printf("[Server] Player %d dir=R\n", playerIdx);
        }
        if (std::strchr(buf, 'U')) slots[playerIdx].vertDir = -1;
        if (std::strchr(buf, 'N')) slots[playerIdx].vertDir = 1;
        if (std::strchr(buf, 'J')) slots[playerIdx].wantsJump = true;
        if (std::strchr(buf, '1')) slots[playerIdx].wantsAttack1 = true;
        if (std::strchr(buf, '2')) slots[playerIdx].wantsAttack2 = true;
        if (std::strchr(buf, '3')) slots[playerIdx].wantsAttack3 = true;
    }

    for (int i = 0; i < 2; ++i) {
        if (!receivedThisTick[i] && slots[i].connected) {
            slots[i].dir = 0;
            slots[i].vertDir = 0;
            slots[i].wantsAttack1 = false;
            slots[i].wantsAttack2 = false;
            slots[i].wantsAttack3 = false;
            slots[i].wantsJump = false;
        }
    }
}

void NetworkManager::broadcastPlayerStatus(int idx, PlayerSlot (&slots)[2],
    uint32_t playerEntityId[2],
    std::unordered_map<uint32_t, CombatantState>& combatants) {
    PlayerStatusMessage msg;
    msg.playerIndex = idx;
    msg.connected = slots[idx].connected && slots[idx].readyInPlayState;

    if (msg.connected && playerEntityId[idx] != 0xFFFFFFFF) {
        auto it = combatants.find(playerEntityId[idx]);
        if (it != combatants.end()) {
            msg.health = it->second.stats.health;
            msg.maxHealth = it->second.stats.maxHealth;
        }
        else {
            msg.health = 100;
            msg.maxHealth = 100;
        }
    }
    else {
        msg.health = 100;
        msg.maxHealth = 100;
    }

    sf::Packet p;
    p << NetMsgType::PlayerStatus << msg;
    for (int i = 0; i < 2; ++i)
        if (slots[i].connected)
            slots[i].tcpSocket.send(p);
}

void NetworkManager::broadcastAllPlayerStatusTo(int toIdx, PlayerSlot (&slots)[2],
    uint32_t playerEntityId[2],
    std::unordered_map<uint32_t, CombatantState>& combatants) {
    if (!slots[toIdx].connected) return;

    for (int i = 0; i < 2; ++i) {
        PlayerStatusMessage msg;
        msg.playerIndex = i;
        msg.connected = slots[i].connected && slots[i].readyInPlayState;

        if (msg.connected && playerEntityId[i] != 0xFFFFFFFF) {
            auto it = combatants.find(playerEntityId[i]);
            if (it != combatants.end()) {
                msg.health = it->second.stats.health;
                msg.maxHealth = it->second.stats.maxHealth;
            }
            else {
                msg.health = 100;
                msg.maxHealth = 100;
            }
        }
        else {
            msg.health = 100;
            msg.maxHealth = 100;
        }

        sf::Packet p;
        p << NetMsgType::PlayerStatus << msg;
        slots[toIdx].tcpSocket.send(p);
    }
}

void NetworkManager::sendAssignPlayerEntity(int idx, PlayerSlot (&slots)[2], uint32_t playerEntityId[2],
    int& playerCount, ServerPhase& phase, WorldManager& world,
    std::unordered_map<uint32_t, CombatantState>& combatants) {
    AssignPlayerMessage assignMsg{ playerEntityId[idx] };
    sf::Packet ap;
    ap << NetMsgType::AssignPlayerEntity << assignMsg;
    if (slots[idx].tcpSocket.send(ap) != sf::Socket::Status::Done) {
        disconnectPlayer(idx, slots, playerCount, phase,world,
            combatants, playerEntityId);
    }
}

void NetworkManager::disconnectPlayer(int idx, PlayerSlot (&slots)[2], int& playerCount,
    ServerPhase& phase, WorldManager& world,
    std::unordered_map<uint32_t, CombatantState>& combatants,
    uint32_t playerEntityId[2]) {
    if (!slots[idx].connected) return;

    if (playerEntityId[idx] != 0xFFFFFFFF) {
        uint32_t entId = playerEntityId[idx];
        playerEntityId[idx] = 0xFFFFFFFF;
        combatants.erase(entId);

        DestroyMessage destroyMsg{ entId };
        for (int i = 0; i < 2; ++i) {
            if (i != idx && slots[i].connected) {
                sf::Packet p;
                p << NetMsgType::DestroyEntity << destroyMsg;
                slots[i].tcpSocket.send(p);
                slots[i].knownEntities.erase(entId);
            }
        }
        slots[idx].knownEntities.erase(entId);
        world.pendingEntityRemovals.push_back(entId);
    }

    slots[idx].tcpSocket.disconnect();
    slots[idx].connected = false;
    slots[idx].ready = false;
    slots[idx].ip = std::nullopt;
    slots[idx].dir = 0;
    slots[idx].vertDir = 0;
    slots[idx].facing = 1;
    slots[idx].knownEntities.clear();
    slots[idx].camX = 0.f;
    slots[idx].readyInPlayState = false;
    slots[idx].isJumping = false;
    slots[idx].idleTicks = 0;
    playerCount--;
    printf("[Server] Player %d disconnected. %d player(s) remaining.\n", idx, playerCount);

    for (int i = 0; i < 2; ++i) {
        if (slots[i].connected) {
            broadcastAllPlayerStatusTo(i, slots, playerEntityId, combatants);
        }
    }

    if (playerCount == 0) {
        phase = ServerPhase::Lobby;
        uint32_t dummyTick = 0;
        world.resetWorld(dummyTick, playerEntityId, slots, combatants);
        printf("[Server] All players gone - world reset. Waiting for new connections.\n");
    }
}