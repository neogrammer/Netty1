#include "WorldManager.h"
#include <network/server/ServerGameLoop.h>
#include <network/server/CombatSystem.h>
#include <cstdio>

void WorldManager::resetWorld(uint32_t& serverTick, uint32_t playerEntityId[2], PlayerSlot (&slots)[2],
    std::unordered_map<uint32_t, CombatantState>& combatants) {
    level.allEntities.clear();
    level.entityIndex.clear();
    nextEntityId = 0;
    serverTick = 0;
    playerEntityId[0] = playerEntityId[1] = 0xFFFFFFFF;
    slots[0].knownEntities.clear();
    slots[1].knownEntities.clear();
    slots[0].camX = 0.f;
    slots[1].camX = 0.f;
    slots[0].readyInPlayState = false;
    slots[1].readyInPlayState = false;
    combatants.clear();
    pendingEntityRemovals.clear();
    printf("[Server] World reset.\n");
}

void WorldManager::initializeWorld(uint32_t& nextEntityIdRef, uint32_t serverTick,
    uint32_t playerEntityId[2], PlayerSlot (&slots)[2],
    std::unordered_map<uint32_t, CombatantState>& combatants,
    CombatSystem& combatSystem) {
    auto spawn = [&](float x, float y, uint8_t anim, EntityType etype) -> uint32_t {
        Entity e;
        e.id = nextEntityIdRef++;
        e.type = etype;
        e.x = x; e.y = y;
        e.animation = anim;
        e.animStartTick = serverTick;
        e.hitbox = { 96.f, 84.f, 74.f, 80.f };
        level.addEntity(e);

        SpawnMessage msg{ e.id, e.type, e.x, e.y, e.animation, e.animStartTick };
        sf::Packet sp;
        sp << NetMsgType::SpawnEntity << msg;
        for (int i = 0; i < 2; ++i)
            if (slots[i].connected)
                slots[i].tcpSocket.send(sp);
        return e.id;
        };

    if (slots[0].connected) {
        playerEntityId[0] = spawn(100.f, 750.f, 0, EntityType::Player);
        combatants[playerEntityId[0]] = combatSystem.createPlayerCombatant();
        slots[0].knownEntities.insert(playerEntityId[0]);
        slots[0].camX = 100.f;
    }

    if (slots[1].connected) {
        playerEntityId[1] = spawn(700.f, 750.f, 0, EntityType::Player);
        combatants[playerEntityId[1]] = combatSystem.createPlayerCombatant();
        slots[1].knownEntities.insert(playerEntityId[1]);
        slots[1].camX = 700.f;
    }

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            if (i != j && slots[i].connected && slots[j].connected) {
                slots[i].knownEntities.insert(playerEntityId[j]);
            }
        }
    }

    this->nextEntityId = nextEntityIdRef;
    printf("[Server] World initialized.\n");
}

void WorldManager::manageEntityVisibility(int playerIdx, PlayerSlot (&slots)[2]) {
    if (playerIdx < 0 || playerIdx >= 2) return;
    if (!slots[playerIdx].connected) return;
    auto& known = slots[playerIdx].knownEntities;
    sf::FloatRect camera(
        { slots[playerIdx].camX - SCREEN_W / 2.f, 0.f },
        { SCREEN_W, SCREEN_H }
    );

    std::vector<uint32_t> toSpawn, toDestroy;

    for (auto& e : level.allEntities) {
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
        if (!ent) { known.erase(id); continue; }
        sendSpawnToPlayer(playerIdx,
            { ent->id, ent->type, ent->x, ent->y, ent->animation, ent->animStartTick }, slots);
    }
    for (auto id : toDestroy) {
        sendDestroyToPlayer(playerIdx, { id }, slots);
    }
}

void WorldManager::buildAndSendSnapshot(int playerIdx, PlayerSlot (&slots)[2], uint32_t serverTick,
    uint32_t playerEntityId[2],
    std::unordered_map<uint32_t, CombatantState>& combatants,
    sf::UdpSocket& udpSocket) {
    if (playerIdx < 0 || playerIdx >= 2) return;
    if (!slots[playerIdx].connected) return;
    if (!slots[playerIdx].ip.has_value()) return;

    FrameSnapshot snap;
    snap.frameNumber = serverTick;
    snap.camX_quant = quantise(slots[playerIdx].camX);
    snap.camY_quant = quantise(0.f);

    std::vector<uint32_t> knownCopy(slots[playerIdx].knownEntities.begin(),
        slots[playerIdx].knownEntities.end());

    for (auto id : knownCopy) {
        auto it = level.entityIndex.find(id);
        if (it == level.entityIndex.end()) {
            slots[playerIdx].knownEntities.erase(id);
            continue;
        }
        Entity* ent = level.getEntity(id);
        if (!ent) {
            slots[playerIdx].knownEntities.erase(id);
            continue;
        }
        Entity& e = *ent;
        EntitySnapshot s;
        s.entityId = e.id;
        s.x_quant = quantise(e.x);
        s.y_quant = quantise(e.y);
        s.animation = e.animation;
        s.animStartTick = e.animStartTick;
        s.flags = (e.id == playerEntityId[0]) ? slots[0].facing
            : (e.id == playerEntityId[1]) ? slots[1].facing : 0;

        auto combatIt = combatants.find(id);
        s.health = (combatIt != combatants.end()) ? combatIt->second.stats.health : 0;

        snap.entities.push_back(s);
    }

    sf::Packet snapPacket;
    snapPacket << NetMsgType::FrameSnapshot << snap;
    udpSocket.send(snapPacket, slots[playerIdx].ip.value(), slots[playerIdx].udpPort);
}

void WorldManager::processRemovals() {
    for (uint32_t id : pendingEntityRemovals) {
        level.removeEntity(id);
    }
    pendingEntityRemovals.clear();
}

void WorldManager::sendSpawnToPlayer(int idx, const SpawnMessage& msg, PlayerSlot (&slots)[2]) {
    if (!slots[idx].connected) return;
    sf::Packet p;
    p << NetMsgType::SpawnEntity << msg;
    if (slots[idx].tcpSocket.send(p) != sf::Socket::Status::Done) {

        auto dummy = std::unordered_map<uint32_t, CombatantState>{};
        disconnectPlayerCallback(idx, slots, dummy);
    }
}

void WorldManager::sendDestroyToPlayer(int idx, const DestroyMessage& msg, PlayerSlot (&slots)[2]) {
    if (!slots[idx].connected) return;
    sf::Packet p;
    p << NetMsgType::DestroyEntity << msg;
    if (slots[idx].tcpSocket.send(p) != sf::Socket::Status::Done) {
        auto dummy = std::unordered_map<uint32_t, CombatantState>{};
        disconnectPlayerCallback(idx, slots, dummy);
    }
}

void WorldManager::disconnectPlayerCallback(int idx, PlayerSlot (&slots)[2],
    std::unordered_map<uint32_t, CombatantState>&) {
    // Stub — the real disconnect is handled by PlayerManager/ServerGameLoop
    // This just avoids compile errors. We'll wire it properly.
}