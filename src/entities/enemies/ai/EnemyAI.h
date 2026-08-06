#pragma once
#include <network/NetTypes.h>
#include <network/server/Combat.h>
#include <entities/enemies/EnemyConfig.h>
#include <cstdint>
#include <unordered_map>
#include <vector>

enum class AIState : uint8_t { Idle, Patrol, Chase, Attack, Hurt, Dead };

struct EnemyAIState {
    AIState state = AIState::Patrol;
    float spawnX = 0.f, spawnY = 0.f;
    float patrolTargetX = 0.f;
    float patrolPauseUntil = 0.f;
    uint32_t lastAttackTick = 0;
    int facing = 1;
    int attackComboStep = 0;
};

class EnemyAIController {
public:
    static void tickAll(
        std::vector<Entity>& entities,
        std::unordered_map<uint32_t, CombatantState>& combatants,
        std::unordered_map<uint32_t, EnemyAIState>& aiStates,
        std::unordered_map<uint32_t, int>& enemyFacing,
        uint32_t serverTick,
        float playerX[2], float playerY[2], bool playerAlive[2],
        const std::unordered_map<EntityType, EnemyConfig>& enemyConfigs);

private:
    static void processEnemy(Entity& e, CombatantState& cs, EnemyAIState& ai,
        uint32_t tick, float pX[2], float pY[2], bool pAlive[2], const EnemyConfig& cfg);
    static int findClosestPlayer(float ex, float ey, float pX[2], float pY[2], bool pAlive[2], float& dist);
    static void moveToward(Entity& e, EnemyAIState& ai, float tx, float ty, float speed, float dt);
};