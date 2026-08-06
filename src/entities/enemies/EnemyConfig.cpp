#include "EnemyConfig.h"

CombatantState EnemyFactory::createGoblinCombatant() {
    CombatantState cs;
    cs.config.baseStats = { 60, 60, 8, 1, 0.05f, 0.8f, 1.f };
    cs.stats = cs.config.baseStats;
    cs.config.attacks = {
        { {4},    { 120.f, 30.f, 70.f, 80.f },  15.f, 0, false },
        { {4, 5}, { 130.f, 10.f, 75.f, 100.f }, 18.f, 0, false },
    };
    cs.isAlive = true;
    return cs;
}

EnemyConfig EnemyFactory::getGoblinConfig() {
    EnemyConfig config;
    config.name = "Goblin";
    config.combatConfig = {
        { 60, 60, 8, 1, 0.05f, 0.8f, 1.f },
        {
            { {4},    { 120.f, 30.f, 70.f, 80.f },  15.f, 0, false },
            { {4, 5}, { 130.f, 10.f, 75.f, 100.f }, 18.f, 0, false },
        }
    };
    config.aiConfig = { 400.f, 120.f, 200.f, 120.f, 1.5f, 3.0f, 1.5f };
    config.hitbox = { 70.f, 65.f, 60.f, 70.f };
    return config;
}

const std::unordered_map<EntityType, EnemyConfig>& EnemyFactory::getEnemyConfigs() {
    static std::unordered_map<EntityType, EnemyConfig> configs = {
        { EntityType::Goblin, getGoblinConfig() }
    };
    return configs;
}