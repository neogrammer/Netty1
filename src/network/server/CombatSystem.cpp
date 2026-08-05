#include "CombatSystem.h"
#include "ServerGameLoop.h"
#include "AnimConfig.h"
#include <cstdio>
#include <cmath>

CombatantState CombatSystem::createPlayerCombatant() {
    CombatantState cs;
    cs.config.baseStats = { 100, 100, 15, 2, 0.1f, 1.f, 1.f };
    cs.stats = cs.config.baseStats;
    cs.config.attacks = {
        { {5}, { 170.f, 10.f, 80.f, 160.f }, 15.f, 0,  false },
        { {4}, { 170.f, 40.f, 80.f, 130.f }, 15.f, 0,  false },
        { {7}, { 170.f, 85.f, 80.f, 80.f },  20.f, 25, true  },
    };
    cs.isAlive = true;
    return cs;
}

int CombatSystem::attackIndex(AnimType type) {
    switch (type) {
    case AnimType::Attack1: return 0;
    case AnimType::Attack2: return 1;
    case AnimType::Attack3: return 2;
    default: return -1;
    }
}

int CombatSystem::calculateDamage(const CombatantState& attacker, const CombatantState& defender) {
    int raw = attacker.stats.strength;
    raw -= defender.stats.defense;
    if (raw < 1) raw = 1;
    raw = static_cast<int>(raw * (1.f - defender.stats.armor));
    if (raw < 1) raw = 1;
    return raw;
}

sf::FloatRect CombatSystem::getStrikeBox(const Entity& entity, const StrikeBox& sb,
    const PlayerSlot (&slots)[2], uint32_t playerEntityId[2]) {
    int facing = 1;
    if (entity.id == playerEntityId[0]) facing = slots[0].facing;
    else if (entity.id == playerEntityId[1]) facing = slots[1].facing;

    float bodyCenterX = entity.x + entity.hitbox.offsetX + entity.hitbox.width / 2.f;
    float y = entity.y + sb.offsetY;

    float strikeX;
    if (facing == 1) {
        strikeX = entity.x + sb.offsetX;
    }
    else {
        float distFromCenter = sb.offsetX - entity.hitbox.offsetX - entity.hitbox.width / 2.f;
        strikeX = bodyCenterX - distFromCenter - sb.width;
    }

    return sf::FloatRect({ strikeX, y }, { sb.width, sb.height });
}

bool CombatSystem::strikeHitsTarget(const sf::FloatRect& strikeBox,
    const Entity& target,
    const Entity& attacker,
    float depthTolerance) {
    Hitbox ha = attacker.hitbox;
    Hitbox hb = target.hitbox;

    float feetAttacker = attacker.y + ha.offsetY + ha.height;
    float feetTarget = target.y + hb.offsetY + hb.height;
    if (std::abs(feetAttacker - feetTarget) > depthTolerance) return false;

    sf::FloatRect targetBox(
        { target.x + hb.offsetX, target.y + hb.offsetY },
        { hb.width, hb.height }
    );

    return strikeBox.findIntersection(targetBox).has_value();
}

void CombatSystem::processAttacks(PlayerSlot (&slots)[2], uint32_t playerEntityId[2],
    Level& level, uint32_t serverTick,
    std::unordered_map<uint32_t, CombatantState>& combatants) {
    for (int i = 0; i < 2; ++i) {
        if (!slots[i].connected) continue;
        if (playerEntityId[i] == 0xFFFFFFFF) continue;
        Entity* attacker = level.getEntity(playerEntityId[i]);
        if (!attacker) continue;

        auto it = combatants.find(playerEntityId[i]);
        if (it == combatants.end()) continue;
        CombatantState& atkState = it->second;

        AnimType cur = static_cast<AnimType>(attacker->animation);
        int idx = attackIndex(cur);
        printf("[Server] processAttacks: player %d, anim=%d, attackIdx=%d\n", i, (int)cur, idx);
        if (idx < 0 || idx >= (int)atkState.config.attacks.size()) continue;

        const HotData& hot = atkState.config.attacks[idx];

        uint32_t elapsed = serverTick - attacker->animStartTick;
        auto animIt = kPlayerAnims.find(cur);
        if (animIt == kPlayerAnims.end()) continue;
        float frameDur = animIt->second.durationPerFrame;
        uint32_t frameTicks = static_cast<uint32_t>(frameDur / (1.0 / 60.0));
        if (frameTicks == 0) frameTicks = 1;
        uint32_t currentFrame = elapsed / frameTicks;
        printf("[Server] Attack check: anim=%d, elapsed=%u, frameDur=%.4f, frameTicks=%u, currentFrame=%u, hotFrames=[",
            (int)cur, elapsed, frameDur, frameTicks, currentFrame);
        for (int hf : hot.hotFrames) printf("%d ", hf);
        printf("]\n");
        bool isHotFrame = false;
        for (int hf : hot.hotFrames) {
            if (currentFrame == (uint32_t)hf) { isHotFrame = true; break; }
        }

        if (!isHotFrame) {
            atkState.lastProcessedFrame = -1;
            continue;
        }

        if (atkState.lastProcessedFrame == (int)currentFrame) continue;
        atkState.lastProcessedFrame = currentFrame;
        printf("[Server] HOT FRAME %u REACHED, lastProcessed=%d, lastAttack=%u, serverTick=%u\n",
            currentFrame, atkState.lastProcessedFrame, atkState.lastAttackTick, serverTick);
        if (atkState.lastAttackTick == serverTick) continue;
        atkState.lastAttackTick = serverTick;
        printf("[Server] Attack tick set, getting strike box...\n");

        sf::FloatRect strikeBox = getStrikeBox(*attacker, hot.strikeBox, slots, playerEntityId);
        printf("[Server] Combatants count: %zu\n", combatants.size());
        for (auto& [targetId, defState] : combatants) {
            if (targetId == playerEntityId[i]) continue;
            if (!defState.isAlive) continue;
            if (serverTick - defState.lastHitTick < 18) continue;

            Entity* target = level.getEntity(targetId);
            if (!target) continue;

            if (strikeHitsTarget(strikeBox, *target, *attacker, hot.depthTolerance)) {
                printf("[Server] HIT! Entity %u hit entity %u\n", attacker->id, target->id);
                int dmg = hot.overrideStrength ? hot.damage : calculateDamage(atkState, defState);
                defState.stats.health -= dmg;
                defState.lastHitTick = serverTick;

                printf("[Server] Entity %u hit entity %u for %d damage (HP: %d)\n",
                    attacker->id, target->id, dmg, defState.stats.health);

                if (defState.stats.health <= 0) {
                    defState.stats.health = 0;
                    defState.isAlive = false;
                    defState.deathTick = serverTick;
                    target->animation = static_cast<uint8_t>(AnimType::Death);
                    target->animStartTick = serverTick;
                }
                else {
                    target->animation = static_cast<uint8_t>(AnimType::Hit);
                    target->animStartTick = serverTick;
                }
            }
            else {
                printf("[Server] Strike missed: attacker=%u target=%u\n", attacker->id, targetId);
            }
        }
    }
}