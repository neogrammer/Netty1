#pragma once
#include <network/NetTypes.h>
#include <network/server/Combat.h>
#include <unordered_map>
#include <string>

struct EnemyAIConfig {
    float aggroRange = 400.f;
    float attackRange = 120.f;
    float patrolDistance = 200.f;
    float moveSpeed = 120.f;
    float patrolPauseMin = 1.5f;
    float patrolPauseMax = 3.0f;
    float attackCooldown = 1.5f;
};

struct EnemyConfig {
    std::string name;
    CombatConfig combatConfig;
    EnemyAIConfig aiConfig;
    Hitbox hitbox;
};

class EnemyFactory {
public:
    static CombatantState createGoblinCombatant();
    static EnemyConfig getGoblinConfig();
    static const std::unordered_map<EntityType, EnemyConfig>& getEnemyConfigs();
    static int16_t getMaxHealth(EntityType type) {
        auto& configs = getEnemyConfigs();
        auto it = configs.find(type);
        return (it != configs.end()) ? it->second.combatConfig.baseStats.maxHealth : 100;
    };
    
};