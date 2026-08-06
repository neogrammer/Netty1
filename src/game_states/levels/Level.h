// Level.h
#pragma once
#include <network/NetTypes.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <res/SceneResources.h>
#include <entities/animation/AnimationSet.h>

class Level {
public:
    std::vector<Entity> allEntities;
    std::unordered_map<uint32_t, size_t> entityIndex;   // entityId → index in allEntities

    void addEntity(Entity e) {
        size_t idx = allEntities.size();
        allEntities.push_back(e);
        entityIndex[e.id] = idx;
    }

    Entity* getEntity(uint32_t id) {
        auto it = entityIndex.find(id);
        if (it == entityIndex.end()) return nullptr;
        if (it->second >= allEntities.size()) return nullptr;
        return &allEntities[it->second];
    }

    // future: load from file
    void loadDefault() {
        // nothing special – entities will be added by the server
    }

    void removeEntity(uint32_t id);

    // Load animation sets for all entity types used in this level
    void loadEntityAnimSets(int levelId, SceneResources& levelRes,
        std::unordered_map<EntityType, AnimationSet*>& animSets);
};