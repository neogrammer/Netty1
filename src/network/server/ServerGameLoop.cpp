#include "ServerGameLoop.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <SFML/System/Sleep.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Clock.hpp>

// ---------- Constructor ----------

ServerGameLoop::ServerGameLoop(sf::TcpListener& lst,
    sf::TcpSocket tcpSocks[2], unsigned short clientPorts[2], int initCount)
    : listener(lst), playerCount(initCount)
{
    printf("sizeof(EntitySnapshot) = %zu\n", sizeof(EntitySnapshot));

    // Move connected sockets into slots
    for (int i = 0; i < 2; ++i) {
        if (i < initCount) {
            slots[i].tcpSocket = std::move(tcpSocks[i]);
            slots[i].udpPort = clientPorts[i];
            slots[i].connected = true;
        }
    }

    // Bind UDP socket
    if (udpSocket.bind(UDP_PORT) != sf::Socket::Status::Done) {
        fprintf(stderr, "[Server] UDP bind failed on port %d\n", UDP_PORT);
        return;
    }
    udpSocket.setBlocking(false);
}

// ---------- Lifecycle ----------

void ServerGameLoop::resetWorld() {
    phase = ServerPhase::Lobby;
    level.allEntities.clear();
    level.entityIndex.clear();
    nextEntityId = 0;
    serverTick = 0;
    playerEntityId[0] = playerEntityId[1] = 0xFFFFFFFF;
    slots[0].knownEntities.clear();
    slots[1].knownEntities.clear();
    slots[0].camX = 0.f;
    slots[1].camX = 0.f;
    printf("[Server] World reset.\n");
}

void ServerGameLoop::disconnectPlayer(int idx) {
    if (!slots[idx].connected) return;
    slots[idx].tcpSocket.disconnect();
    slots[idx].connected = false;
    slots[idx].ready = false;
    slots[idx].ip = std::nullopt;
    slots[idx].dir = 0;
    slots[idx].facing = 1;
    slots[idx].knownEntities.clear();
    slots[idx].camX = 0.f;
    playerCount--;
    printf("[Server] Player %d disconnected. %d player(s) remaining.\n", idx, playerCount);
    if (playerCount == 0) {
        resetWorld();
        printf("[Server] All players gone – world reset. Waiting for new connections.\n");
    }
}

void ServerGameLoop::initializeWorld() {
    phase = ServerPhase::Playing;

    auto spawn = [&](float x, float y, uint8_t anim, EntityType etype) -> uint32_t {
        Entity e;
        e.id = nextEntityId++;
        e.type = etype;
        e.x = x; e.y = y;
        e.animation = anim;
        e.animStartTick = serverTick;
        level.addEntity(e);

        SpawnMessage msg{ e.id, e.type, e.x, e.y, e.animation, e.animStartTick };
        sf::Packet sp;
        sp << NetMsgType::SpawnEntity << msg;
        for (int i = 0; i < 2; ++i)
            if (slots[i].connected)
                slots[i].tcpSocket.send(sp);
        return e.id;
        };

    playerEntityId[0] = spawn(100.f, 700.f, 0, EntityType::Player);
    playerEntityId[1] = spawn(700.f, 700.f, 0, EntityType::Player);

    for (int i = 0; i < 2; ++i) {
        slots[i].knownEntities.insert(playerEntityId[0]);
        slots[i].knownEntities.insert(playerEntityId[1]);
        slots[i].camX = (i == 0) ? 100.f : 700.f;
    }
    printf("[Server] World initialized.\n");
}

// ---------- Networking Helpers ----------

void ServerGameLoop::sendSpawnToPlayer(int idx, const SpawnMessage& msg) {
    if (!slots[idx].connected) return;
    sf::Packet p;
    p << NetMsgType::SpawnEntity << msg;
    if (slots[idx].tcpSocket.send(p) != sf::Socket::Status::Done)
        disconnectPlayer(idx);
}

void ServerGameLoop::sendDestroyToPlayer(int idx, const DestroyMessage& msg) {
    if (!slots[idx].connected) return;
    sf::Packet p;
    p << NetMsgType::DestroyEntity << msg;
    if (slots[idx].tcpSocket.send(p) != sf::Socket::Status::Done)
        disconnectPlayer(idx);
}

void ServerGameLoop::sendAssignPlayerEntity(int idx) {
    AssignPlayerMessage assignMsg{ playerEntityId[idx] };
    sf::Packet ap;
    ap << NetMsgType::AssignPlayerEntity << assignMsg;
    if (slots[idx].tcpSocket.send(ap) != sf::Socket::Status::Done)
        disconnectPlayer(idx);
}

// ---------- UDP Handshake ----------

void ServerGameLoop::waitForUdpHandshake() {
    printf("[Server] Waiting for UDP 'OK' from connected players...\n");

    while (!slots[0].ip.has_value() && !slots[1].ip.has_value()) {
        // Try to receive UDP "OK" from known ports
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
                        if (slots[i].tcpSocket.send(confirm) != sf::Socket::Status::Done)
                            disconnectPlayer(i);
                        break;
                    }
                }
            }
        }

        // Accept late joins during handshake
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

                    // Wait for UDP OK from this player
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
                    if (slots[freeSlot].tcpSocket.send(confirm) != sf::Socket::Status::Done)
                        disconnectPlayer(freeSlot);

                    if (playerCount == 1 && phase == ServerPhase::Lobby)
                        initializeWorld();
                }
            }
        }

        if (slots[0].ip.has_value() || slots[1].ip.has_value())
            break;

        sf::sleep(sf::milliseconds(10));
    }
}

// ---------- Late Join (in-game) ----------

void ServerGameLoop::acceptLateJoin() {
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

    // Wait for UDP OK
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

    // Confirm
    sf::Packet confirm;
    std::string okStr = "OK";
    confirm << okStr;
    if (slots[freeSlot].tcpSocket.send(confirm) != sf::Socket::Status::Done) {
        disconnectPlayer(freeSlot);
        return;
    }

    // Initialize world if this is the first player
    if (playerCount == 1 && phase == ServerPhase::Lobby)
        initializeWorld();

    // Assign player entity
    sendAssignPlayerEntity(freeSlot);

    // Send all existing entities to the new player
    for (auto& e : level.allEntities) {
        SpawnMessage spawnMsg{ e.id, e.type, e.x, e.y, e.animation, e.animStartTick };
        sf::Packet sp;
        sp << NetMsgType::SpawnEntity << spawnMsg;
        if (slots[freeSlot].tcpSocket.send(sp) != sf::Socket::Status::Done) {
            disconnectPlayer(freeSlot);
            break;
        }
    }
}

// ---------- Input Processing ----------

void ServerGameLoop::processPlayerInput() {
    char buf[64];
    std::size_t received;
    std::optional<sf::IpAddress> senderIp;
    unsigned short senderPort;

    while (udpSocket.receive(buf, sizeof(buf) - 1, received, senderIp, senderPort)
        == sf::Socket::Status::Done) {
        buf[received] = '\0';

        int playerIdx = (senderPort == slots[0].udpPort) ? 0
            : (senderPort == slots[1].udpPort) ? 1
            : -1;
        if (playerIdx < 0 || !slots[playerIdx].connected) continue;

        if (std::strcmp(buf, "READY") == 0) {
            if (!slots[playerIdx].ready) {
                slots[playerIdx].ready = true;
                printf("[Server] Player %d is ready – sending LoadLevel\n", playerIdx);
                LoadLevelMessage levelMsg{ 1 };
                sf::Packet p;
                p << NetMsgType::LoadLevel << levelMsg;
                if (slots[playerIdx].tcpSocket.send(p) != sf::Socket::Status::Done)
                    disconnectPlayer(playerIdx);
            }
            continue;
        }

        if (std::strchr(buf, 'L')) slots[playerIdx].dir = -1;
        if (std::strchr(buf, 'R')) slots[playerIdx].dir = 1;
    }
}

// ---------- Game Tick ----------

void ServerGameLoop::tickGameLogic() {
    // Update facing from input direction
    for (int i = 0; i < 2; ++i) {
        if (slots[i].dir != 0)
            slots[i].facing = (slots[i].dir > 0) ? 1 : 0;
    }

    // Move player entities and update camera
    for (auto& e : level.allEntities) {
        for (int i = 0; i < 2; ++i) {
            if (e.id != playerEntityId[i]) continue;

            // Apply movement
            e.x += static_cast<float>(slots[i].dir * PLAYER_SPEED * TICK_DURATION);

            // Update animation
            uint8_t newAnim = (slots[i].dir == 0) ? 0 : 1;
            if (newAnim != e.animation) {
                e.animation = newAnim;
                e.animStartTick = static_cast<uint32_t>(serverTick);
            }
            slots[i].dir = 0;

            // --- Dead-zone camera ---
            float camX = slots[i].camX;
            if (camX == 0.f) camX = e.x;

            float deadLeft = camX - SCREEN_W / 2.f + DEAD_ZONE;
            float deadRight = camX + SCREEN_W / 2.f - DEAD_ZONE;

            if (e.x < deadLeft)
                camX = e.x + (SCREEN_W / 2.f - DEAD_ZONE);
            else if (e.x > deadRight)
                camX = e.x - (SCREEN_W / 2.f - DEAD_ZONE);

            // Clamp camera to world boundaries
            if (camX - SCREEN_W / 2.f < LEFT_WORLD)
                camX = LEFT_WORLD + SCREEN_W / 2.f;
            if (camX + SCREEN_W / 2.f > RIGHT_WORLD)
                camX = RIGHT_WORLD - SCREEN_W / 2.f;

            // Clamp player to world boundaries
            if (e.x < LEFT_WORLD)  e.x = LEFT_WORLD;
            if (e.x > RIGHT_WORLD) e.x = RIGHT_WORLD;

            slots[i].camX = camX;
        }
    }
}

void ServerGameLoop::manageEntityVisibility(int playerIdx) {
    auto& known = slots[playerIdx].knownEntities;
    sf::FloatRect camera(
        { slots[playerIdx].camX - SCREEN_W / 2.f, 0.f },
        { SCREEN_W, SCREEN_H }
    );

    std::vector<uint32_t> toSpawn, toDestroy;

    for (auto& e : level.allEntities) {
        // Players are always known
        if (e.type == EntityType::Player) {
            if (!known.count(e.id)) {
                toSpawn.push_back(e.id);
                known.insert(e.id);
            }
            continue;
        }

        bool visible = camera.contains({ e.x, e.y });
        bool alreadyKnown = known.count(e.id) > 0;

        if (visible && !alreadyKnown) {
            toSpawn.push_back(e.id);
            known.insert(e.id);
        }
        else if (!visible && alreadyKnown) {
            toDestroy.push_back(e.id);
            known.erase(e.id);
        }
    }

    for (auto id : toSpawn) {
        Entity* ent = level.getEntity(id);
        if (ent)
            sendSpawnToPlayer(playerIdx,
                { ent->id, ent->type, ent->x, ent->y, ent->animation, ent->animStartTick });
    }
    for (auto id : toDestroy) {
        sendDestroyToPlayer(playerIdx, { id });
    }
}

void ServerGameLoop::buildAndSendSnapshot(int playerIdx) {
    if (!slots[playerIdx].ip.has_value()) return;

    FrameSnapshot snap;
    snap.frameNumber = serverTick;
    snap.camX_quant = quantise(slots[playerIdx].camX);
    snap.camY_quant = quantise(0.f);

    auto& known = slots[playerIdx].knownEntities;
    for (auto id : known) {
        auto it = level.entityIndex.find(id);
        if (it == level.entityIndex.end()) {
            known.erase(id);
            continue;
        }

        Entity& e = *level.getEntity(id);
        EntitySnapshot s;
        s.entityId = e.id;
        s.x_quant = quantise(e.x);
        s.y_quant = quantise(e.y);
        s.animation = e.animation;
        s.animStartTick = e.animStartTick;
        s.flags = (e.id == playerEntityId[0]) ? slots[0].facing
            : (e.id == playerEntityId[1]) ? slots[1].facing : 0;
        snap.entities.push_back(s);
    }

    sf::Packet snapPacket;
    snapPacket << NetMsgType::FrameSnapshot << snap;
    udpSocket.send(snapPacket, slots[playerIdx].ip.value(), slots[playerIdx].udpPort);
}

// ---------- Main Loop ----------

void ServerGameLoop::run() {
    // Phase 1: UDP handshake
    waitForUdpHandshake();

    // Phase 2: Initialize world if needed
    if (phase == ServerPhase::Lobby && playerCount >= 1)
        initializeWorld();

    // Phase 3: Assign player entities
    for (int i = 0; i < 2; ++i) {
        if (slots[i].connected)
            sendAssignPlayerEntity(i);
    }

    printf("[Server] At least one player ready. Starting game...\n");

    // Phase 4: Main game loop
    const sf::Time TICK_TIME = sf::seconds(static_cast<float>(TICK_DURATION));
    sf::Clock gameClock;
    sf::Time accumulator = sf::Time::Zero;
    sf::Time previousTime = gameClock.getElapsedTime();

    while (running) {
        sf::Time now = gameClock.getElapsedTime();
        sf::Time dt = now - previousTime;
        previousTime = now;
        accumulator += dt;

        // Check for TCP disconnects
        for (int i = 0; i < 2; ++i) {
            if (!slots[i].connected) continue;
            char dummy[1]; std::size_t dummyRecv = 0;
            if (slots[i].tcpSocket.receive(dummy, 0, dummyRecv)
                == sf::Socket::Status::Disconnected) {
                printf("[Server] TCP disconnect detected for player %d\n", i);
                disconnectPlayer(i);
            }
        }

        // Accept late joins
        if (playerCount < 2)
            acceptLateJoin();

        // Process incoming input
        processPlayerInput();

        // Fixed-timestep updates
        while (accumulator >= TICK_TIME) {
            accumulator -= TICK_TIME;
            serverTick++;

            if (phase == ServerPhase::Playing) {
                tickGameLogic();

                for (int i = 0; i < 2; ++i) {
                    if (!slots[i].connected || !slots[i].ip.has_value()) continue;
                    manageEntityVisibility(i);
                    buildAndSendSnapshot(i);
                }
            }
        }

        // Precise sleep to maintain frame pacing
        sf::Time nextTick = previousTime + TICK_TIME - sf::microseconds(500);
        now = gameClock.getElapsedTime();
        if (nextTick > now)
            sf::sleep(nextTick - now);
    }


    // Cleanup
    printf("[Server] Shutting down...\n");
    for (int i = 0; i < 2; ++i)
        if (slots[i].connected)
            slots[i].tcpSocket.disconnect();
    udpSocket.unbind();
    printf("[Server] Finished.\n");
}

void ServerGameLoop::shutdown() {
    running = false;
}