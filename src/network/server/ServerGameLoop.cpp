//#include "ServerGameLoop.h"
//#include <cstdio>
//#include <cstring>
//#include <algorithm>
//#include <SFML/System/Sleep.hpp>
//#include <SFML/Graphics/Rect.hpp>
//#include <SFML/System/Clock.hpp>
//#include "AnimConfig.h"
//
//
//
//// ---------- Constructor ----------
//
//ServerGameLoop::ServerGameLoop(sf::TcpListener& lst,
//    sf::TcpSocket tcpSocks[2], unsigned short clientPorts[2], int initCount)
//    : listener(lst), playerCount(initCount)
//{
//    printf("sizeof(EntitySnapshot) = %zu\n", sizeof(EntitySnapshot));
//
//    // Move connected sockets into slots
//    for (int i = 0; i < 2; ++i) {
//        if (i < initCount) {
//            slots[i].tcpSocket = std::move(tcpSocks[i]);
//            slots[i].udpPort = clientPorts[i];
//            slots[i].connected = true;
//        }
//    }
//
//    // Bind UDP socket
//    if (udpSocket.bind(UDP_PORT) != sf::Socket::Status::Done) {
//        fprintf(stderr, "[Server] UDP bind failed on port %d\n", UDP_PORT);
//        return;
//    }
//    udpSocket.setBlocking(false);
//}
//
//// ---------- Lifecycle ----------
//
//void ServerGameLoop::resetWorld() {
//    phase = ServerPhase::Lobby;
//    level.allEntities.clear();
//    level.entityIndex.clear();
//    nextEntityId = 0;
//    serverTick = 0;
//    playerEntityId[0] = playerEntityId[1] = 0xFFFFFFFF;
//    slots[0].knownEntities.clear();
//    slots[1].knownEntities.clear();
//    slots[0].camX = 0.f;
//    slots[1].camX = 0.f;
//    slots[0].readyInPlayState = false;
//    slots[1].readyInPlayState = false;
//    combatants.clear();
//    pendingEntityRemovals.clear();
//    printf("[Server] World reset.\n");
//
//
//}
//
//void ServerGameLoop::disconnectPlayer(int idx) {
//    if (!slots[idx].connected) return;
//
//    // --- Queue entity for deferred removal ---
//    if (playerEntityId[idx] != 0xFFFFFFFF) {
//        uint32_t entId = playerEntityId[idx];
//        playerEntityId[idx] = 0xFFFFFFFF;
//        combatants.erase(entId);
//
//        DestroyMessage destroyMsg{ entId };
//        for (int i = 0; i < 2; ++i) {
//            if (i != idx && slots[i].connected) {
//                sendDestroyToPlayer(i, destroyMsg);
//                slots[i].knownEntities.erase(entId);
//            }
//        }
//        // Also remove from disconnected player's own known set (about to be cleared anyway)
//        slots[idx].knownEntities.erase(entId);
//        pendingEntityRemovals.push_back(entId);
//    }
//
//    slots[idx].tcpSocket.disconnect();
//    slots[idx].connected = false;
//    slots[idx].ready = false;
//    slots[idx].ip = std::nullopt;
//    slots[idx].dir = 0;
//    slots[idx].vertDir = 0;
//    slots[idx].facing = 1;
//    slots[idx].knownEntities.clear();
//    slots[idx].camX = 0.f;
//    slots[idx].readyInPlayState = false;
//    slots[idx].isJumping = false;
//    slots[idx].idleTicks = 0;
//    playerCount--;
//    printf("[Server] Player %d disconnected. %d player(s) remaining.\n", idx, playerCount);
//
//    // Notify remaining player about the disconnect
//    for (int i = 0; i < 2; ++i) {
//        if (slots[i].connected) {
//            broadcastAllPlayerStatusTo(i);
//        }
//    }
//
//    if (playerCount == 0) {
//        resetWorld();
//        printf("[Server] All players gone – world reset. Waiting for new connections.\n");
//    }
//}
// 
//void ServerGameLoop::initializeWorld() {
//    phase = ServerPhase::Playing;
//
//    auto spawn = [&](float x, float y, uint8_t anim, EntityType etype) -> uint32_t {
//        Entity e;
//        e.id = nextEntityId++;
//        e.type = etype;
//        e.x = x; e.y = y;
//        e.animation = anim;
//        e.animStartTick = serverTick;
//        e.hitbox = { 96.f, 84.f, 74.f, 80.f };
//        level.addEntity(e);
//
//        SpawnMessage msg{ e.id, e.type, e.x, e.y, e.animation, e.animStartTick };
//        sf::Packet sp;
//        sp << NetMsgType::SpawnEntity << msg;
//        for (int i = 0; i < 2; ++i)
//            if (slots[i].connected)
//                slots[i].tcpSocket.send(sp);
//        return e.id;
//        };
//
//    // Only spawn for connected players
//    if (slots[0].connected) {
//        playerEntityId[0] = spawn(100.f, 750.f, 0, EntityType::Player);
//        combatants[playerEntityId[0]] = createPlayerCombatant();
//        slots[0].knownEntities.insert(playerEntityId[0]);
//        slots[0].camX = 100.f;
//    }
//
//    if (slots[1].connected) {
//        playerEntityId[1] = spawn(700.f, 750.f, 0, EntityType::Player);
//        combatants[playerEntityId[1]] = createPlayerCombatant();
//        slots[1].knownEntities.insert(playerEntityId[1]);
//        slots[1].camX = 700.f;
//    }
//
//    // Cross-register: each connected player knows about the other's entity
//    for (int i = 0; i < 2; ++i) {
//        for (int j = 0; j < 2; ++j) {
//            if (i != j && slots[i].connected && slots[j].connected) {
//                slots[i].knownEntities.insert(playerEntityId[j]);
//            }
//        }
//    }
//
//    printf("[Server] World initialized.\n");
//}
//
//// ---------- Networking Helpers ----------
//
//void ServerGameLoop::sendSpawnToPlayer(int idx, const SpawnMessage& msg) {
//    if (!slots[idx].connected) return;
//    sf::Packet p;
//    p << NetMsgType::SpawnEntity << msg;
//    if (slots[idx].tcpSocket.send(p) != sf::Socket::Status::Done)
//        disconnectPlayer(idx);
//}
//
//void ServerGameLoop::sendDestroyToPlayer(int idx, const DestroyMessage& msg) {
//    if (!slots[idx].connected) return;
//    sf::Packet p;
//    p << NetMsgType::DestroyEntity << msg;
//    if (slots[idx].tcpSocket.send(p) != sf::Socket::Status::Done)
//        disconnectPlayer(idx);
//}
//
//void ServerGameLoop::sendAssignPlayerEntity(int idx) {
//    AssignPlayerMessage assignMsg{ playerEntityId[idx] };
//    sf::Packet ap;
//    ap << NetMsgType::AssignPlayerEntity << assignMsg;
//    if (slots[idx].tcpSocket.send(ap) != sf::Socket::Status::Done)
//        disconnectPlayer(idx);
//}
//
//// ---------- UDP Handshake ----------
//
//void ServerGameLoop::waitForUdpHandshake() {
//    printf("[Server] Waiting for UDP 'OK' from connected players...\n");
//
//    while (!slots[0].ip.has_value() && !slots[1].ip.has_value()) {
//        // Try to receive UDP "OK" from known ports
//        char buf[64];
//        std::size_t received;
//        std::optional<sf::IpAddress> senderIp;
//        unsigned short senderPort;
//
//        while (udpSocket.receive(buf, sizeof(buf) - 1, received, senderIp, senderPort)
//            == sf::Socket::Status::Done) {
//            buf[received] = '\0';
//            if (std::strcmp(buf, "OK") == 0) {
//                for (int i = 0; i < 2; ++i) {
//                    if (slots[i].connected && senderPort == slots[i].udpPort
//                        && !slots[i].ip.has_value()) {
//                        slots[i].ip = senderIp;
//                        printf("[Server] Learned endpoint for player %d: %s:%d\n",
//                            i, senderIp->toString().c_str(), senderPort);
//
//                        sf::Packet confirm;
//                        std::string ok = "OK";
//                        confirm << ok;
//                        if (slots[i].tcpSocket.send(confirm) != sf::Socket::Status::Done)
//                            disconnectPlayer(i);
//                        break;
//                    }
//                }
//            }
//        }
//
//        // Accept late joins during handshake
//        if (playerCount < 2) {
//            sf::TcpSocket newSocket;
//            if (listener.accept(newSocket) == sf::Socket::Status::Done) {
//                int freeSlot = -1;
//                for (int i = 0; i < 2; ++i) {
//                    if (!slots[i].connected) { freeSlot = i; break; }
//                }
//                if (freeSlot >= 0) {
//                    unsigned short newUdpPort;
//                    sf::Packet pkt;
//                    pkt << freeSlot << static_cast<unsigned short>(UDP_PORT);
//                    if (newSocket.send(pkt) != sf::Socket::Status::Done) continue;
//                    pkt.clear();
//                    if (newSocket.receive(pkt) != sf::Socket::Status::Done) continue;
//                    pkt >> newUdpPort;
//
//                    slots[freeSlot].tcpSocket = std::move(newSocket);
//                    slots[freeSlot].tcpSocket.setBlocking(false);
//                    slots[freeSlot].udpPort = newUdpPort;
//                    slots[freeSlot].connected = true;
//                    playerCount++;
//                    printf("[Server] Late player %d registered during handshake (UDP %d)\n",
//                        freeSlot, newUdpPort);
//
//                    // Wait for UDP OK from this player
//                    char okBuf[4]; std::size_t recvd;
//                    std::optional<sf::IpAddress> ip; unsigned short port;
//                    while (udpSocket.receive(okBuf, 3, recvd, ip, port)
//                        != sf::Socket::Status::Done
//                        || std::strncmp(okBuf, "OK", 2) != 0
//                        || port != newUdpPort) {
//                        sf::sleep(sf::milliseconds(10));
//                    }
//                    slots[freeSlot].ip = ip;
//                    printf("[Server] Learned endpoint for late player %d: %s:%d\n",
//                        freeSlot, ip->toString().c_str(), port);
//
//                    sf::Packet confirm;
//                    std::string okStr = "OK";
//                    confirm << okStr;
//                    if (slots[freeSlot].tcpSocket.send(confirm) != sf::Socket::Status::Done)
//                        disconnectPlayer(freeSlot);
//
//                    if (playerCount == 1 && phase == ServerPhase::Lobby)
//                        initializeWorld();
//                }
//            }
//        }
//
//        if (slots[0].ip.has_value() || slots[1].ip.has_value())
//            break;
//
//        sf::sleep(sf::milliseconds(10));
//    }
//}
//
//
//// ---------- Input Processing ----------
//
//void ServerGameLoop::processPlayerInput() {
//    char buf[64];
//    std::size_t received;
//    std::optional<sf::IpAddress> senderIp;
//    unsigned short senderPort;
//
//    bool receivedThisTick[2] = { false, false };   // <-- HERE, at top of method
//
//    while (udpSocket.receive(buf, sizeof(buf) - 1, received, senderIp, senderPort)
//        == sf::Socket::Status::Done) {
//        buf[received] = '\0';
//
//        int playerIdx = (senderPort == slots[0].udpPort) ? 0
//            : (senderPort == slots[1].udpPort) ? 1
//            : -1;
//        if (playerIdx < 0 || !slots[playerIdx].connected) continue;
//
//        receivedThisTick[playerIdx] = true;   
//
//        //if (std::strcmp(buf, "READY") == 0) {
//        //    if (!slots[playerIdx].ready) {
//        //        slots[playerIdx].ready = true;
//        //        printf("[Server] Player %d is ready – sending LoadLevel\n", playerIdx);
//        //        LoadLevelMessage levelMsg{ 1 };
//        //        sf::Packet p;
//        //        p << NetMsgType::LoadLevel << levelMsg;
//        //        if (slots[playerIdx].tcpSocket.send(p) != sf::Socket::Status::Done)
//        //            disconnectPlayer(playerIdx);
//        //    }
//        //    continue;
//        //}
//
//        if (std::strcmp(buf, "READY") == 0) {
//            if (!slots[playerIdx].ready) {
//                slots[playerIdx].ready = true;
//
//                // Spawn the player's entity if it doesn't exist yet (late join)
//                if (playerEntityId[playerIdx] == 0xFFFFFFFF) {
//                    Entity e;
//                    e.id = nextEntityId++;
//                    e.type = EntityType::Player;
//                    e.x = (playerIdx == 0) ? 100.f : 700.f;
//                    e.y = 750.f;
//                    e.animation = static_cast<uint8_t>(AnimType::Idle);
//                    e.animStartTick = serverTick;
//                    e.hitbox = { 96.f, 84.f, 74.f, 80.f };
//                    level.addEntity(e);
//                    playerEntityId[playerIdx] = e.id;
//                    combatants[e.id] = createPlayerCombatant();
//                    slots[playerIdx].knownEntities.insert(e.id);
//                    slots[playerIdx].camX = e.x;
//
//                    // Send AssignPlayerEntity
//                    sendAssignPlayerEntity(playerIdx);
//
//                    // Send all existing entities to the new player
//                    for (auto& ent : level.allEntities) {
//                        SpawnMessage spawnMsg{ ent.id, ent.type, ent.x, ent.y, ent.animation, ent.animStartTick };
//                        sf::Packet sp;
//                        sp << NetMsgType::SpawnEntity << spawnMsg;
//                        slots[playerIdx].tcpSocket.send(sp);
//                    }
//
//                    // Send the new player's entity to existing players
//                    SpawnMessage newPlayerMsg{ e.id, e.type, e.x, e.y, e.animation, e.animStartTick };
//                    for (int i = 0; i < 2; ++i) {
//                        if (i != playerIdx && slots[i].connected) {
//                            sendSpawnToPlayer(i, newPlayerMsg);
//                            slots[i].knownEntities.insert(e.id);
//                        }
//                    }
//
//                    // Cross-register known entities
//                    for (int i = 0; i < 2; ++i) {
//                        if (slots[i].connected && playerEntityId[i] != 0xFFFFFFFF) {
//                            slots[playerIdx].knownEntities.insert(playerEntityId[i]);
//                            if (i != playerIdx) {
//                                slots[i].knownEntities.insert(playerEntityId[playerIdx]);
//                            }
//                        }
//                    }
//                }
//
//                printf("[Server] Player %d is ready – sending LoadLevel\n", playerIdx);
//                LoadLevelMessage levelMsg{ 1 };
//                sf::Packet p;
//                p << NetMsgType::LoadLevel << levelMsg;
//                if (slots[playerIdx].tcpSocket.send(p) != sf::Socket::Status::Done)
//                    disconnectPlayer(playerIdx);
//            }
//            continue;
//        }
//
//
//        // Reset action flags for this player
//        slots[playerIdx].dir = 0;
//        slots[playerIdx].vertDir = 0;
//        slots[playerIdx].wantsAttack1 = false;
//        slots[playerIdx].wantsAttack2 = false;
//        slots[playerIdx].wantsAttack3 = false;
//        slots[playerIdx].wantsJump = false;
//
//        // Parse input
//        if (std::strchr(buf, 'L')) slots[playerIdx].dir = -1;
//        if (std::strchr(buf, 'R')) slots[playerIdx].dir = 1;
//        if (std::strchr(buf, 'U')) slots[playerIdx].vertDir = -1;   // up (into background)
//        if (std::strchr(buf, 'N')) slots[playerIdx].vertDir = 1;    // down (toward foreground)
//        if (std::strchr(buf, 'J')) slots[playerIdx].wantsJump = true;
//        if (std::strchr(buf, '1')) slots[playerIdx].wantsAttack1 = true;
//        if (std::strchr(buf, '2')) slots[playerIdx].wantsAttack2 = true;
//        if (std::strchr(buf, '3')) slots[playerIdx].wantsAttack3 = true;
//    }
//
//    // HERE, after the while loop — reset only players who didn't send a packet
//    for (int i = 0; i < 2; ++i) {
//        if (!receivedThisTick[i] && slots[i].connected) {
//            slots[i].dir = 0;
//            slots[i].vertDir = 0;
//            slots[i].wantsAttack1 = false;
//            slots[i].wantsAttack2 = false;
//            slots[i].wantsAttack3 = false;
//            slots[i].wantsJump = false;
//        }
//    }
//}
//// ---------- Game Tick ----------
//
//void ServerGameLoop::tickGameLogic() {
//
//    // Update facing from input direction
//    for (int i = 0; i < 2; ++i) {
//        if (slots[i].dir != 0)
//            slots[i].facing = (slots[i].dir > 0) ? 1 : 0;
//    }
//
//    // Move player entities and update camera
//    for (auto& e : level.allEntities) {
//        for (int i = 0; i < 2; ++i) {
//            if (playerEntityId[i] == 0xFFFFFFFF) continue;
//            if (e.id != playerEntityId[i]) continue;
//
//            auto& slot = slots[i];
//
//            // Track idle time for grace period
//            if (slot.dir == 0 && !slot.wantsAttack1 && !slot.wantsAttack2 &&
//                !slot.wantsAttack3 && !slot.wantsJump) {
//                slot.idleTicks++;
//            }
//            else {
//                slot.idleTicks = 0;
//            }
//
//            // --- Determine animation ---
//            //
//            
//            AnimType current = static_cast<AnimType>(e.animation);
//            
//            bool inAttack = (current == AnimType::Attack1 ||
//                current == AnimType::Attack2 ||
//                current == AnimType::Attack3);
//            bool inJump = (current == AnimType::JumpUp || current == AnimType::JumpDown);
//
//            // Check if current non-looping animation finished
//            uint32_t elapsed = serverTick - e.animStartTick;
//            bool animFinished = !animLoops(current) && elapsed >= animDurationTicks(current);
//            
//            AnimType desired = current;  // default: stay in current
//            
//            // Jump phase timing
//            uint32_t jumpElapsed = serverTick - slot.jumpStartTick;
//
//            // --- Jump transition handling ---
//            if (slot.isJumping && current == AnimType::JumpUp && jumpElapsed >= JUMP_UP_DURATION) {
//                // Rising phase complete — switch to falling
//                desired = AnimType::JumpDown;
//            }
//            else if (slot.isJumping && current == AnimType::JumpDown && jumpElapsed >= JUMP_TOTAL_DURATION) {
//                // Full jump complete
//                desired = AnimType::Idle;
//                slot.isJumping = false;
//            }
//            else if (slot.isJumping) {
//                // Still in jump — keep current phase
//                desired = current;
//            }
//            else if (current == AnimType::Death) {
//                // dead — no changes
//            }
//            else if (animFinished && inAttack) {
//                // Attack ended — check for combo input
//                if (slot.wantsAttack2 && current == AnimType::Attack1) {
//                    desired = AnimType::Attack2;
//                }
//                else if (slot.wantsAttack3 && current == AnimType::Attack2) {
//                    desired = AnimType::Attack3;
//                }
//                else if (slot.idleTicks >= 8) {
//                    desired = AnimType::Idle;
//                }
//                else {
//                    desired = AnimType::Walk;  // keep last known movement
//                }
//            }
//            else if (!inAttack && !inJump) {
//                // Not in any special animation — process input
//                if (slot.wantsAttack1) {
//                    desired = AnimType::Attack1;
//                    printf("[Server] Player %d wants Attack1\n", i);
//                }
//                else if (slot.wantsAttack2) desired = AnimType::Attack2;
//                else if (slot.wantsAttack3) desired = AnimType::Attack3;
//                else if (slot.wantsJump) {
//                    desired = AnimType::JumpUp;
//                    slot.isJumping = true;
//                    slot.jumpStartTick = serverTick;
//                    slot.jumpStartY = e.y;
//                    printf("[Server] Player %d jump started at Y=%.1f\n", i, e.y);
//                }
//                else if (slot.dir != 0)     desired = AnimType::Walk;
//                else if (slot.idleTicks >= 8) desired = AnimType::Idle;
//                else                        desired = AnimType::Walk;  // keep moving
//            }
//
//            if (desired != current) {
//                printf("[Server] Player %d anim change: %d -> %d (tick %u, idleTicks=%u)\n",
//                    i, current, desired, serverTick, slot.idleTicks);
//
//                e.animation = static_cast<uint8_t>(desired);
//                e.animStartTick = serverTick;
//            }
//
//            // Facing
//            if (slot.dir != 0) slot.facing = (slot.dir > 0) ? 1 : 0;
//
//            // Movement
//            bool canMove = (desired == AnimType::Idle || desired == AnimType::Walk ||
//                desired == AnimType::JumpUp || desired == AnimType::JumpDown);
//            if (canMove) {
//                e.x += slot.dir * PLAYER_SPEED * TICK_DURATION;
//
//                // Vertical movement (only when grounded)
//                if (!slot.isJumping) {
//                    e.y += slot.vertDir * PLAYER_SPEED * TICK_DURATION;
//                    if (e.y < 530.f) e.y = 530.f;
//                    if (e.y > 628.f) e.y = 628.f;
//                    if (slot.vertDir != 0) {
//                        printf("[Server] Player %d Y: %.1f (vertDir=%d)\n", i, e.y, slot.vertDir);
//                    }
//                }
//            }
//
//            // Vertical movement during jump 
//            if (desired == AnimType::JumpUp) {
//                float jumpProgress = (float)(serverTick - slot.jumpStartTick) / JUMP_UP_DURATION;
//                e.y = slot.jumpStartY - 120.f * (1.f - (1.f - jumpProgress) * (1.f - jumpProgress));
//            }
//            else if (desired == AnimType::JumpDown) {
//                float fallProgress = (float)(serverTick - slot.jumpStartTick - JUMP_UP_DURATION) / JUMP_DOWN_DURATION;
//                e.y = slot.jumpStartY - 120.f + 120.f * fallProgress * fallProgress;
//            }
//            else if (!slot.isJumping) {
//                // Grounded Y is maintained by vertical movement above
//            }
//
//            // --- Dead-zone camera ---
//            float camX = slots[i].camX;
//            if (camX == 0.f) camX = e.x;
//
//            float deadLeft = camX - SCREEN_W / 2.f + DEAD_ZONE;
//            float deadRight = camX + SCREEN_W / 2.f - DEAD_ZONE;
//
//            if (e.x < deadLeft)
//                camX = e.x + (SCREEN_W / 2.f - DEAD_ZONE);
//            else if (e.x > deadRight)
//                camX = e.x - (SCREEN_W / 2.f - DEAD_ZONE);
//
//            // Clamp camera to world boundaries
//            if (camX - SCREEN_W / 2.f < LEFT_WORLD)
//                camX = LEFT_WORLD + SCREEN_W / 2.f;
//            if (camX + SCREEN_W / 2.f > RIGHT_WORLD)
//                camX = RIGHT_WORLD - SCREEN_W / 2.f;
//
//            // Clamp player to world boundaries
//            if (e.x < LEFT_WORLD)  e.x = LEFT_WORLD;
//            if (e.x > RIGHT_WORLD) e.x = RIGHT_WORLD;
//
//            slots[i].camX = camX;
//        }
//    }
//}
//
//void ServerGameLoop::manageEntityVisibility(int playerIdx) {
//    if (playerIdx < 0 || playerIdx >= 2) return;
//    if (!slots[playerIdx].connected) return;
//    auto& known = slots[playerIdx].knownEntities;
//    sf::FloatRect camera(
//        { slots[playerIdx].camX - SCREEN_W / 2.f, 0.f },
//        { SCREEN_W, SCREEN_H }
//    );
//
//    std::vector<uint32_t> toSpawn, toDestroy;
//
//    for (auto& e : level.allEntities) {
//        // Players are always known
//        if (e.type == EntityType::Player) {
//            if (!known.count(e.id)) {
//                toSpawn.push_back(e.id);
//                known.insert(e.id);
//            }
//            continue;
//        }
//
//        bool visible = camera.contains({ e.x, e.y });
//        bool alreadyKnown = known.count(e.id) > 0;
//
//        if (visible && !alreadyKnown) {
//            toSpawn.push_back(e.id);
//            known.insert(e.id);
//        }
//        else if (!visible && alreadyKnown) {
//            toDestroy.push_back(e.id);
//            known.erase(e.id);
//        }
//    }
//
//    for (auto id : toSpawn) {
//        Entity* ent = level.getEntity(id);
//        if (!ent) {
//            known.erase(id);
//            continue;
//        }
//        if (ent)
//            sendSpawnToPlayer(playerIdx,
//                { ent->id, ent->type, ent->x, ent->y, ent->animation, ent->animStartTick });
//    }
//    for (auto id : toDestroy) {
//        sendDestroyToPlayer(playerIdx, { id });
//    }
//}
//
//
//void ServerGameLoop::buildAndSendSnapshot(int playerIdx) {
//    if (playerIdx < 0 || playerIdx >= 2) return;
//    if (!slots[playerIdx].connected) return;
//    if (!slots[playerIdx].ip.has_value()) return;
//
//    FrameSnapshot snap;
//    snap.frameNumber = serverTick;
//    snap.camX_quant = quantise(slots[playerIdx].camX);
//    snap.camY_quant = quantise(0.f);
//
//    // Copy the known set to avoid iterator invalidation
//    std::vector<uint32_t> knownCopy(slots[playerIdx].knownEntities.begin(),
//        slots[playerIdx].knownEntities.end());
//
//    for (auto id : knownCopy) {
//        auto it = level.entityIndex.find(id);
//        if (it == level.entityIndex.end()) {
//            slots[playerIdx].knownEntities.erase(id);
//            continue;
//        }
//        Entity* ent = level.getEntity(id);
//        if (!ent) {
//            slots[playerIdx].knownEntities.erase(id);
//            continue;
//        }
//        Entity& e = *ent;
//        EntitySnapshot s;
//        s.entityId = e.id;
//        s.x_quant = quantise(e.x);
//        s.y_quant = quantise(e.y);
//        s.animation = e.animation;
//        s.animStartTick = e.animStartTick;
//        s.flags = (e.id == playerEntityId[0]) ? slots[0].facing
//            : (e.id == playerEntityId[1]) ? slots[1].facing : 0;
//
//        auto combatIt = combatants.find(id);
//        s.health = (combatIt != combatants.end()) ? combatIt->second.stats.health : 0;
//
//        snap.entities.push_back(s);
//    }
//
//    sf::Packet snapPacket;
//    snapPacket << NetMsgType::FrameSnapshot << snap;
//    udpSocket.send(snapPacket, slots[playerIdx].ip.value(), slots[playerIdx].udpPort);
//}
//
//void ServerGameLoop::processAttacks() {
//
//
//    for (int i = 0; i < 2; ++i) {
//        if (!slots[i].connected) continue;
//        if (playerEntityId[i] == 0xFFFFFFFF) continue;
//        Entity* attacker = level.getEntity(playerEntityId[i]);
//        if (!attacker) continue;
//
//        auto it = combatants.find(playerEntityId[i]);
//        if (it == combatants.end()) continue;
//        CombatantState& atkState = it->second;
//
//        AnimType cur = static_cast<AnimType>(attacker->animation);
//        int idx = attackIndex(cur);
//        if (idx < 0 || idx >= (int)atkState.config.attacks.size()) continue;
//
//        const HotData& hot = atkState.config.attacks[idx];
//
//        // Hot frame check
//        uint32_t elapsed = serverTick - attacker->animStartTick;
//        auto animIt = kPlayerAnims.find(cur);
//        if (animIt == kPlayerAnims.end()) continue;
//        float frameDur = animIt->second.durationPerFrame;
//        uint32_t frameTicks = static_cast<uint32_t>(frameDur / TICK_DURATION);
//        if (frameTicks == 0) frameTicks = 1;
//        uint32_t currentFrame = elapsed / frameTicks;
//
//        bool isHotFrame = false;
//        for (int hf : hot.hotFrames) {
//            if (currentFrame == (uint32_t)hf) { isHotFrame = true; break; }
//        }
//
//        // Only process if this is a hot frame AND we haven't processed it yet for this attack
//        if (!isHotFrame) {
//            atkState.lastProcessedFrame = -1;  // reset when we leave hot frames
//            continue;
//        }
//
//        if (atkState.lastProcessedFrame == (int)currentFrame) continue;  // already hit on this frame
//        atkState.lastProcessedFrame = currentFrame;
//
//        // Prevent double-hit this tick
//        if (atkState.lastAttackTick == serverTick) continue;
//        atkState.lastAttackTick = serverTick;
//
//        printf("[Server] Player %d Attack%d HOT FRAME %u FIRED (tick %u)\n",
//            i, idx + 1, currentFrame, serverTick);
//
//        // Strike box in world space
//        sf::FloatRect strikeBox = getStrikeBox(*attacker, hot.strikeBox);
//
//        // Check all targets
//        for (auto& [targetId, defState] : combatants) {
//            if (targetId == playerEntityId[i]) continue;
//            if (!defState.isAlive) continue;
//            if (serverTick - defState.lastHitTick < 18) continue;  // i-frames
//
//            Entity* target = level.getEntity(targetId);
//          
//            if (!target) continue;
//
//            if (strikeHitsTarget(strikeBox, *target, *attacker, hot.depthTolerance)) {
//                int dmg = hot.overrideStrength ? hot.damage : calculateDamage(atkState, defState);
//                defState.stats.health -= dmg;
//                defState.lastHitTick = serverTick;
//
//                printf("[Server] Entity %u hit entity %u for %d damage (HP: %d)\n",
//                    attacker->id, target->id, dmg, defState.stats.health);
//
//                if (defState.stats.health <= 0) {
//                    defState.stats.health = 0;
//                    defState.isAlive = false;
//                    defState.deathTick = serverTick;
//                    target->animation = static_cast<uint8_t>(AnimType::Death);
//                    target->animStartTick = serverTick;
//                }
//                else {
//                    target->animation = static_cast<uint8_t>(AnimType::Hit);
//                    target->animStartTick = serverTick;
//                }
//            }
//        }
//    }
//}
//
//
//// ---------- Main Loop ----------
//
//void ServerGameLoop::run() {
//    // Phase 1: UDP handshake
//    waitForUdpHandshake();
//
//    // Phase 2: Initialize world if needed
//    if (phase == ServerPhase::Lobby && playerCount >= 1)
//        initializeWorld();
//
//    // Phase 3: Assign player entities
//    for (int i = 0; i < 2; ++i) {
//        if (slots[i].connected)
//            sendAssignPlayerEntity(i);
//    }
//
//    printf("[Server] At least one player ready. Starting game...\n");
//
//    // Phase 4: Main game loop
//    const sf::Time TICK_TIME = sf::seconds(static_cast<float>(TICK_DURATION));
//    sf::Clock gameClock;
//    sf::Time accumulator = sf::Time::Zero;
//    sf::Time previousTime = gameClock.getElapsedTime();
//
//    while (running) {
//        sf::Time now = gameClock.getElapsedTime();
//        sf::Time dt = now - previousTime;
//        previousTime = now;
//        accumulator += dt;
//
//        // Check for TCP messages and disconnects
//        for (int i = 0; i < 2; ++i) {
//            if (!slots[i].connected) continue;
//
//            sf::Packet tcpPacket;
//            auto status = slots[i].tcpSocket.receive(tcpPacket);
//            if (status == sf::Socket::Status::Disconnected) {
//                printf("[Server] TCP disconnect detected for player %d\n", i);
//                disconnectPlayer(i);
//            }
//            else if (status == sf::Socket::Status::Done) {
//                NetMsgType type;
//                tcpPacket >> type;
//                if (type == NetMsgType::PlayerReady) {
//                    printf("[Server] Player %d is ready in PlayState\n", i);
//                    slots[i].readyInPlayState = true;
//
//                    // Broadcast status to all connected players
//                    bool bothReady = true;
//                    for (int j = 0; j < 2; ++j) {
//                        if (slots[j].connected && !slots[j].readyInPlayState) {
//                            bothReady = false;
//                            break;
//                        }
//                    }
//
//                    if (bothReady || playerCount == 1) {
//                        for (int j = 0; j < 2; ++j) {
//                            if (slots[j].connected) {
//                                broadcastPlayerStatus(j);
//                            }
//                        }
//                    }
//                    else {
//                        broadcastPlayerStatus(i);
//                    }
//                }
//            }
//        }
//
//        // Accept late joins
//        if (playerCount < 2)
//            acceptLateJoin();
//
//        // Process incoming input
//        processPlayerInput();
//
//        // Fixed-timestep updates
//        while (accumulator >= TICK_TIME) {
//            accumulator -= TICK_TIME;
//            serverTick++;
//
//            // Process pending entity removals FIRST
//            for (uint32_t id : pendingEntityRemovals) {
//                level.removeEntity(id);
//            }
//            pendingEntityRemovals.clear();
//
//            if (phase == ServerPhase::Playing) {
//                tickGameLogic();
//                processAttacks();
//                for (int i = 0; i < 2; ++i) {
//                    if (!slots[i].connected || !slots[i].ip.has_value()) continue;
//                    manageEntityVisibility(i);
//                    buildAndSendSnapshot(i);
//                }
//            }
//        }
//
//        // Precise sleep to maintain frame pacing
//        sf::Time nextTick = previousTime + TICK_TIME - sf::microseconds(500);
//        now = gameClock.getElapsedTime();
//        if (nextTick > now)
//            sf::sleep(nextTick - now);
//    }
//
//    // Cleanup
//    printf("[Server] Shutting down...\n");
//    for (int i = 0; i < 2; ++i)
//        if (slots[i].connected)
//            slots[i].tcpSocket.disconnect();
//    udpSocket.unbind();
//    printf("[Server] Finished.\n");
//}
//
//void ServerGameLoop::shutdown() {
//    running = false;
//}
//
//
//bool ServerGameLoop::hitboxesOverlap(const Entity& a, const Entity& b) {
//    Hitbox ha = a.hitbox;
//    Hitbox hb = b.hitbox;
//
//    // Feet Y for both entities
//    float feetA = a.y + ha.offsetY + ha.height;
//    float feetB = b.y + hb.offsetY + hb.height;
//
//    // Depth check: feet must be within 10 pixels of each other
//    if (std::abs(feetA - feetB) > 10.f) return false;
//
//    // If attacker is facing left, flip the hitbox X offset
//    int facingA = 1;
//    if (a.id == playerEntityId[0]) facingA = slots[0].facing;
//    else if (a.id == playerEntityId[1]) facingA = slots[1].facing;
//    // For enemies, they'll have their own facing — default to 1 for now
//
//    float ax = a.x + (facingA == 1 ? ha.offsetX : ha.offsetX - ha.width);
//    float ay = a.y + ha.offsetY;
//    float bx = b.x + hb.offsetX;
//    float by = b.y + hb.offsetY;
//
//    return (ax < bx + hb.width && ax + ha.width > bx &&
//        ay < by + hb.height && ay + ha.height > by);
//}
//
//int ServerGameLoop::attackIndex(AnimType type) {
//    switch (type) {
//    case AnimType::Attack1: return 0;
//    case AnimType::Attack2: return 1;
//    case AnimType::Attack3: return 2;
//    default: return -1;
//    }
//}
//
//
//CombatantState ServerGameLoop::createPlayerCombatant() {
//    CombatantState cs;
//    cs.config.baseStats = { 100, 100, 15, 2, 0.1f, 1.f, 1.f };
//    cs.stats = cs.config.baseStats;
//    cs.config.attacks = {
//        { {5}, { 170.f, 10.f, 80.f, 160.f }, 15.f, 0,  false },
//        { {4}, { 170.f, 40.f, 80.f, 130.f }, 15.f, 0,  false },
//        { {7}, { 170.f, 85.f, 80.f, 80.f },  20.f, 25, true  },
//    };
//    cs.isAlive = true;
//    return cs;
//}
//
//
//int ServerGameLoop::calculateDamage(const CombatantState& attacker, const CombatantState& defender) {
//    int raw = attacker.stats.strength;
//    raw -= defender.stats.defense;
//    if (raw < 1) raw = 1;
//    raw = static_cast<int>(raw * (1.f - defender.stats.armor));
//    if (raw < 1) raw = 1;
//    return raw;
//}
//
//sf::FloatRect ServerGameLoop::getStrikeBox(const Entity& entity, const StrikeBox& sb) {
//    int facing = 1;
//    if (entity.id == playerEntityId[0]) facing = slots[0].facing;
//    else if (entity.id == playerEntityId[1]) facing = slots[1].facing;
//
//    float bodyCenterX = entity.x + entity.hitbox.offsetX + entity.hitbox.width / 2.f;
//    float y = entity.y + sb.offsetY;
//
//    float strikeX;
//    if (facing == 1) {
//        strikeX = entity.x + sb.offsetX;
//    }
//    else {
//        // Mirror: distance from body center to strike box start, flipped
//        float distFromCenter = sb.offsetX - entity.hitbox.offsetX - entity.hitbox.width / 2.f;
//        strikeX = bodyCenterX - distFromCenter - sb.width;
//    }
//
//    return sf::FloatRect({ strikeX, y }, { sb.width, sb.height });
//}
//bool ServerGameLoop::strikeHitsTarget(const sf::FloatRect& strikeBox,
//    const Entity& target,
//    const Entity& attacker,
//    float depthTolerance) {
//    Hitbox ha = attacker.hitbox;
//    Hitbox hb = target.hitbox;
//
//    float feetAttacker = attacker.y + ha.offsetY + ha.height;
//    float feetTarget = target.y + hb.offsetY + hb.height;
//    if (std::abs(feetAttacker - feetTarget) > depthTolerance) return false;
//
//    sf::FloatRect targetBox(
//        { target.x + hb.offsetX, target.y + hb.offsetY },
//        { hb.width, hb.height }
//    );
//
//    return strikeBox.findIntersection(targetBox).has_value();
//}
//
//
//
//
//void ServerGameLoop::broadcastPlayerStatus(int idx) {
//    PlayerStatusMessage msg;
//    msg.playerIndex = idx;
//    msg.connected = slots[idx].connected && slots[idx].readyInPlayState;
//
//    if (msg.connected && playerEntityId[idx] != 0xFFFFFFFF) {
//        auto it = combatants.find(playerEntityId[idx]);
//        if (it != combatants.end()) {
//            msg.health = it->second.stats.health;
//            msg.maxHealth = it->second.stats.maxHealth;
//        }
//        else {
//            msg.health = 100;
//            msg.maxHealth = 100;
//        }
//    }
//    else {
//        msg.health = 100;
//        msg.maxHealth = 100;
//    }
//
//    sf::Packet p;
//    p << NetMsgType::PlayerStatus << msg;
//    for (int i = 0; i < 2; ++i)
//        if (slots[i].connected)
//            slots[i].tcpSocket.send(p);
//}
//
//void ServerGameLoop::broadcastAllPlayerStatusTo(int toIdx) {
//    if (!slots[toIdx].connected) return;
//
//    for (int i = 0; i < 2; ++i) {
//        PlayerStatusMessage msg;
//        msg.playerIndex = i;
//        msg.connected = slots[i].connected && slots[i].readyInPlayState;
//
//        if (msg.connected && playerEntityId[i] != 0xFFFFFFFF) {
//            auto it = combatants.find(playerEntityId[i]);
//            if (it != combatants.end()) {
//                msg.health = it->second.stats.health;
//                msg.maxHealth = it->second.stats.maxHealth;
//            }
//            else {
//                msg.health = 100;
//                msg.maxHealth = 100;
//            }
//        }
//        else {
//            msg.health = 100;
//            msg.maxHealth = 100;
//        }
//
//        sf::Packet p;
//        p << NetMsgType::PlayerStatus << msg;
//        slots[toIdx].tcpSocket.send(p);
//    }
//}
//
//void ServerGameLoop::acceptLateJoin() {
//    sf::TcpSocket newSocket;
//    if (listener.accept(newSocket) != sf::Socket::Status::Done) return;
//
//    int freeSlot = -1;
//    for (int i = 0; i < 2; ++i) {
//        if (!slots[i].connected) { freeSlot = i; break; }
//    }
//    if (freeSlot < 0) return;
//
//    unsigned short newUdpPort;
//    sf::Packet pkt;
//    pkt << freeSlot << static_cast<unsigned short>(UDP_PORT);
//    if (newSocket.send(pkt) != sf::Socket::Status::Done) return;
//    pkt.clear();
//    if (newSocket.receive(pkt) != sf::Socket::Status::Done) return;
//    pkt >> newUdpPort;
//
//    slots[freeSlot].tcpSocket = std::move(newSocket);
//    slots[freeSlot].tcpSocket.setBlocking(false);
//    slots[freeSlot].udpPort = newUdpPort;
//    slots[freeSlot].connected = true;
//    playerCount++;
//    printf("[Server] Late player %d joined (UDP %d)\n", freeSlot, newUdpPort);
//
//    // Wait for UDP OK
//    char okBuf[4]; std::size_t recvd;
//    std::optional<sf::IpAddress> ip; unsigned short port;
//    while (udpSocket.receive(okBuf, 3, recvd, ip, port) != sf::Socket::Status::Done
//        || std::strncmp(okBuf, "OK", 2) != 0
//        || port != newUdpPort) {
//        sf::sleep(sf::milliseconds(10));
//    }
//    slots[freeSlot].ip = ip;
//    printf("[Server] Learned endpoint for late player %d: %s:%d\n",
//        freeSlot, ip->toString().c_str(), port);
//
//    // Confirm
//    sf::Packet confirm;
//    std::string okStr = "OK";
//    confirm << okStr;
//    if (slots[freeSlot].tcpSocket.send(confirm) != sf::Socket::Status::Done) {
//        disconnectPlayer(freeSlot);
//        return;
//    }
//
//    // Initialize world if this is the first player
//    if (playerCount == 1 && phase == ServerPhase::Lobby)
//        initializeWorld();
//
//    // Notify all players about connection status
//    for (int i = 0; i < 2; ++i) {
//        if (slots[i].connected) {
//            broadcastAllPlayerStatusTo(i);
//        }
//    }
//
//    printf("[Server] Late player %d fully initialized\n", freeSlot);
//}


#include "ServerGameLoop.h"
#include "AnimConfig.h"
#include <SFML/System/Clock.hpp>
#include <SFML/System/Sleep.hpp>
#include <cstdio>

ServerGameLoop::ServerGameLoop(sf::TcpListener& lst,
    sf::TcpSocket tcpSocks[2], unsigned short clientPorts[2], int initCount)
    : network{ lst }
{
    printf("sizeof(EntitySnapshot) = %zu\n", sizeof(EntitySnapshot));

    for (int i = 0; i < 2; ++i) {
        if (i < initCount) {
            players.slots[i].tcpSocket = std::move(tcpSocks[i]);
            players.slots[i].udpPort = clientPorts[i];
            players.slots[i].connected = true;
        }
    }
    players.playerCount = initCount;
}

void ServerGameLoop::tickGameLogic() {
    for (int i = 0; i < 2; ++i) {
        if (players.slots[i].dir != 0)
            players.slots[i].facing = (players.slots[i].dir > 0) ? 1 : 0;
    }

    for (auto& e : world.level.allEntities) {
        for (int i = 0; i < 2; ++i) {
            if (players.playerEntityId[i] == 0xFFFFFFFF) continue;
            if (e.id != players.playerEntityId[i]) continue;

            auto& slot = players.slots[i];
            if (slot.dir != 0) printf("[Server] tickGameLogic: player %d dir=%d\n", i, slot.dir);
            AnimType desired = animController.updateAnimation(slot, e, serverTick);

            if (slot.dir != 0) slot.facing = (slot.dir > 0) ? 1 : 0;

            bool canMove = (desired == AnimType::Idle || desired == AnimType::Walk ||
                desired == AnimType::JumpUp || desired == AnimType::JumpDown);
            if (canMove) {
                e.x += slot.dir * PLAYER_SPEED * (float)TICK_DURATION;

                if (!slot.isJumping) {
                    e.y += slot.vertDir * PLAYER_SPEED * (float)TICK_DURATION;
                    if (e.y < 530.f) e.y = 530.f;
                    if (e.y > 628.f) e.y = 628.f;
                }
            }

            if (desired == AnimType::JumpUp) {
                float jumpProgress = (float)(serverTick - slot.jumpStartTick) / AnimationController::JUMP_UP_DURATION;
                e.y = slot.jumpStartY - 120.f * (1.f - (1.f - jumpProgress) * (1.f - jumpProgress));
            }
            else if (desired == AnimType::JumpDown) {
                float fallProgress = (float)(serverTick - slot.jumpStartTick - AnimationController::JUMP_UP_DURATION)
                    / AnimationController::JUMP_DOWN_DURATION;
                e.y = slot.jumpStartY - 120.f + 120.f * fallProgress * fallProgress;
            }

            updateCamera(slot, e, i);
        }
    }
}

void ServerGameLoop::updateCamera(PlayerSlot& slot, Entity& entity, int idx) {
    float camX = slot.camX;
    if (camX == 0.f) camX = entity.x;

    float deadLeft = camX - SCREEN_W / 2.f + DEAD_ZONE;
    float deadRight = camX + SCREEN_W / 2.f - DEAD_ZONE;

    if (entity.x < deadLeft)
        camX = entity.x + (SCREEN_W / 2.f - DEAD_ZONE);
    else if (entity.x > deadRight)
        camX = entity.x - (SCREEN_W / 2.f - DEAD_ZONE);

    if (camX - SCREEN_W / 2.f < LEFT_WORLD)  camX = LEFT_WORLD + SCREEN_W / 2.f;
    if (camX + SCREEN_W / 2.f > RIGHT_WORLD) camX = RIGHT_WORLD - SCREEN_W / 2.f;

    if (entity.x < LEFT_WORLD)  entity.x = LEFT_WORLD;
    if (entity.x > RIGHT_WORLD) entity.x = RIGHT_WORLD;

    slot.camX = camX;
}

void ServerGameLoop::run() {
    network.waitForUdpHandshake(players.slots, players.playerCount, phase, world, combatants, combat, players.playerEntityId);

    //if (phase == ServerPhase::Lobby && players.playerCount >= 1) {
    //    world.initializeWorld(world.nextEntityId, serverTick, players.playerEntityId,
    //        players.slots, combatants, combat);
    //    phase = ServerPhase::Playing;
    //    printf("[Server] World re-initialized after reset\n");
    //    printf("[Server] combatants size after init: %zu\n", combatants.size());
    //    for (auto& [id, cs] : combatants) {
    //        printf("[Server]   combatant %u (alive=%d)\n", id, cs.isAlive);
    //    }
    //}

    for (int i = 0; i < 2; ++i) {
        if (players.slots[i].connected)
            network.sendAssignPlayerEntity(i, players.slots, players.playerEntityId, players.playerCount, phase, world, combatants);
    }

    printf("[Server] At least one player ready. Starting game...\n");

    const sf::Time TICK_TIME = sf::seconds(static_cast<float>(TICK_DURATION));
    sf::Clock gameClock;
    sf::Time accumulator = sf::Time::Zero;
    sf::Time previousTime = gameClock.getElapsedTime();

    while (running) {
        sf::Time now = gameClock.getElapsedTime();
        sf::Time dt = now - previousTime;
        previousTime = now;
        accumulator += dt;

        for (int i = 0; i < 2; ++i) {
            if (!players.slots[i].connected) continue;
            sf::Packet tcpPacket;
            auto status = players.slots[i].tcpSocket.receive(tcpPacket);
            if (status == sf::Socket::Status::Disconnected) {
                printf("[Server] TCP disconnect detected for player %d\n", i);
                network.disconnectPlayer(i, players.slots, players.playerCount,
                    phase, world, combatants, players.playerEntityId);
            }
            else if (status == sf::Socket::Status::Done) {
                NetMsgType type;
                tcpPacket >> type;
                if (type == NetMsgType::PlayerReady) {
                    printf("[Server] Player %d is ready in PlayState\n", i);
                    players.slots[i].readyInPlayState = true;

                    bool bothReady = true;
                    for (int j = 0; j < 2; ++j) {
                        if (players.slots[j].connected && !players.slots[j].readyInPlayState) {
                            bothReady = false;
                            break;
                        }
                    }

                    if (bothReady || players.playerCount == 1) {
                        for (int j = 0; j < 2; ++j) {
                            if (players.slots[j].connected) {
                                network.broadcastPlayerStatus(j, players.slots,
                                    players.playerEntityId, combatants);
                            }
                        }
                    }
                    else {
                        network.broadcastPlayerStatus(i, players.slots,
                            players.playerEntityId, combatants);
                    }
                }
            }
        }

        if (players.playerCount < 2)
            network.acceptLateJoin(players.slots, players.playerCount, phase, world,
                combat, world.nextEntityId, serverTick,
                players.playerEntityId, combatants);

        // ADD THIS BLOCK:
        if (phase == ServerPhase::Lobby && players.playerCount >= 1) {
            world.initializeWorld(world.nextEntityId, serverTick, players.playerEntityId,
                players.slots, combatants, combat);
            phase = ServerPhase::Playing;
            for (int i = 0; i < 2; ++i) {
                if (players.slots[i].connected)
                    network.sendAssignPlayerEntity(i, players.slots, players.playerEntityId,
                        players.playerCount, phase, world, combatants);
            }
            printf("[Server] World re-initialized after reset\n");
        }


        network.processPlayerInput(players.slots, world, combat, world.nextEntityId,
            serverTick, players.playerEntityId, players.playerCount,
            phase, combatants);

        while (accumulator >= TICK_TIME) {
            accumulator -= TICK_TIME;
            serverTick++;

            world.processRemovals();

            if (phase == ServerPhase::Playing) {
                static int tickCount = 0;
                if (++tickCount % 60 == 0) printf("[Server] Tick loop running, phase=Playing, tick=%u\n", serverTick);
                tickGameLogic();
                combat.processAttacks(players.slots, players.playerEntityId,
                    world.level, serverTick, combatants);
                for (int i = 0; i < 2; ++i) {
                    if (!players.slots[i].connected || !players.slots[i].ip.has_value()) continue;
                    world.manageEntityVisibility(i, players.slots);
                    world.buildAndSendSnapshot(i, players.slots, serverTick,
                        players.playerEntityId, combatants,
                        network.udpSocket);
                }
            }
            else {
                static int notPlayingCount = 0;
                if (++notPlayingCount % 60 == 0) printf("[Server] Tick loop SKIPPED, phase=%d\n", (int)phase);
            }
        }

        sf::Time nextTick = previousTime + TICK_TIME - sf::microseconds(500);
        now = gameClock.getElapsedTime();
        if (nextTick > now)
            sf::sleep(nextTick - now);
    }

    printf("[Server] Shutting down...\n");
    for (int i = 0; i < 2; ++i)
        if (players.slots[i].connected)
            players.slots[i].tcpSocket.disconnect();
    network.udpSocket.unbind();
    printf("[Server] Finished.\n");
}

void ServerGameLoop::shutdown() {
    running = false;
}