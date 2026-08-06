#include "EnemyAI.h"
#include <network/server/AnimConfig.h>
#include <cmath>
#include <algorithm>
#include <cstdlib>

void EnemyAIController::tickAll(
    std::vector<Entity>& entities,
    std::unordered_map<uint32_t, CombatantState>& combatants,
    std::unordered_map<uint32_t, EnemyAIState>& aiStates,
    std::unordered_map<uint32_t, int>& enemyFacing,
    uint32_t serverTick,
    float playerX[2], float playerY[2], bool playerAlive[2],
    const std::unordered_map<EntityType, EnemyConfig>& enemyConfigs)
{
    for (auto& entity : entities) {
        if (entity.type == EntityType::Player) continue;
        auto combatIt = combatants.find(entity.id);
        if (combatIt == combatants.end()) continue;
        auto configIt = enemyConfigs.find(entity.type);
        if (configIt == enemyConfigs.end()) continue;

        if (aiStates.find(entity.id) == aiStates.end()) {
            EnemyAIState ai;
            ai.spawnX = entity.x;
            ai.spawnY = entity.y;
            ai.patrolTargetX = entity.x + 150.f;
            aiStates[entity.id] = ai;
        }

        processEnemy(entity, combatIt->second, aiStates[entity.id],
            serverTick, playerX, playerY, playerAlive, configIt->second);
        enemyFacing[entity.id] = aiStates[entity.id].facing;
    }
}

void EnemyAIController::processEnemy(
    Entity& entity, CombatantState& combatant, EnemyAIState& ai,
    uint32_t serverTick, float playerX[2], float playerY[2],
    bool playerAlive[2], const EnemyConfig& config)
{
    float dt = 1.0f / 60.0f;
    float serverTime = serverTick * dt;
    const auto& aic = config.aiConfig;

    if (!combatant.isAlive) {
        if (ai.state != AIState::Dead) {
            ai.state = AIState::Dead;
            entity.animation = (uint8_t)AnimType::Death;
            entity.animStartTick = serverTick;
        }
        return;
    }

    AnimType cur = (AnimType)entity.animation;

    // Hitstun
    if (cur == AnimType::Hit) {
        auto& tbl = getAnimTable(entity.type);
        auto it = tbl.find(AnimType::Hit);
        if (it != tbl.end() && (serverTick - entity.animStartTick) * dt >= it->second.frameCount * it->second.durationPerFrame) {
            ai.state = AIState::Chase;
            entity.animation = (uint8_t)AnimType::Idle;
            entity.animStartTick = serverTick;
        }
        return;
    }

    // Don't interrupt attacks
    if (cur == AnimType::Attack1 || cur == AnimType::Attack2) {
        auto& tbl = getAnimTable(entity.type);
        auto it = tbl.find(cur);
        if (it != tbl.end() && (serverTick - entity.animStartTick) * dt < it->second.frameCount * it->second.durationPerFrame)
            return;
        ai.state = AIState::Chase;
        entity.animation = (uint8_t)AnimType::Idle;
        entity.animStartTick = serverTick;
    }

    float closestDist;
    int target = findClosestPlayer(entity.x, entity.y, playerX, playerY, playerAlive, closestDist);

    switch (ai.state) {
    case AIState::Patrol:
        if (target >= 0 && closestDist < aic.aggroRange) { ai.state = AIState::Chase; break; }
        if (std::abs(entity.x - ai.patrolTargetX) < 10.f) {
            if (serverTime >= ai.patrolPauseUntil) {
                float off = (rand() % 2 ? 1.f : -1.f) * (aic.patrolDistance * 0.5f + (float)rand() / RAND_MAX * aic.patrolDistance);
                ai.patrolTargetX = std::clamp(ai.spawnX + off, 50.f, 17950.f);
                ai.patrolPauseUntil = serverTime + aic.patrolPauseMin + (float)rand() / RAND_MAX * (aic.patrolPauseMax - aic.patrolPauseMin);
                entity.animation = (uint8_t)AnimType::Idle;
                entity.animStartTick = serverTick;
            }
        }
        else {
            moveToward(entity, ai, ai.patrolTargetX, entity.y, aic.moveSpeed, dt);
            if (cur != AnimType::Walk) { entity.animation = (uint8_t)AnimType::Walk; entity.animStartTick = serverTick; }
        }
        break;

    case AIState::Chase:
        if (target < 0 || closestDist > aic.aggroRange * 1.5f) { ai.state = AIState::Patrol; ai.patrolTargetX = entity.x; break; }
        if (closestDist <= aic.attackRange) { ai.state = AIState::Attack; break; }
        moveToward(entity, ai, playerX[target], entity.y, aic.moveSpeed * 1.3f, dt);
        if (cur != AnimType::Walk) { entity.animation = (uint8_t)AnimType::Walk; entity.animStartTick = serverTick; }
        break;

    case AIState::Attack:
        if (target < 0 || closestDist > aic.attackRange * 1.3f) { ai.state = AIState::Chase; break; }
        ai.facing = (playerX[target] > entity.x) ? 1 : 0;
        if (serverTime - ai.lastAttackTick * dt >= aic.attackCooldown) {
            ai.lastAttackTick = serverTick;
            entity.animation = (ai.attackComboStep == 0) ? (uint8_t)AnimType::Attack1 : (uint8_t)AnimType::Attack2;
            ai.attackComboStep = 1 - ai.attackComboStep;
            entity.animStartTick = serverTick;
        }
        else if (cur != AnimType::Idle) {
            entity.animation = (uint8_t)AnimType::Idle;
            entity.animStartTick = serverTick;
        }
        break;

    default: break;
    }
}

int EnemyAIController::findClosestPlayer(float ex, float ey, float pX[2], float pY[2], bool pAlive[2], float& dist) {
    int best = -1;
    float minD = 999999.f;
    for (int i = 0; i < 2; ++i) {
        if (!pAlive[i]) continue;
        float d = std::hypot(ex - pX[i], ey - pY[i]);
        if (d < minD) { minD = d; best = i; }
    }
    dist = minD;
    return best;
}

void EnemyAIController::moveToward(Entity& e, EnemyAIState& ai, float tx, float ty, float speed, float dt) {
    float dx = tx - e.x, dy = ty - e.y;
    float dist = std::hypot(dx, dy);
    if (dist < 1.f) return;
    e.x += dx / dist * speed * dt;
    if (std::abs(dy) > 5.f) { e.y += dy / dist * speed * dt; e.y = std::clamp(e.y, 530.f, 628.f); }
    ai.facing = (dx > 0) ? 1 : 0;
    e.x = std::clamp(e.x, 50.f, 17950.f);
}