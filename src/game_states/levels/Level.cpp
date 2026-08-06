#include "Level.h"
#include <entities/animation/enemies/GoblinAnimations.h>
#include <res/Cfg.h>

void Level::removeEntity(uint32_t id) {
    auto it = entityIndex.find(id);
    if (it != entityIndex.end()) {
        allEntities.erase(
            std::remove_if(allEntities.begin(), allEntities.end(),
                [id](const Entity& e) { return e.id == id; }),
            allEntities.end()
        );
        entityIndex.erase(it);
    }
}

void Level::loadEntityAnimSets(int levelId, SceneResources& levelRes, std::unordered_map<EntityType, AnimationSet*>& animSets)
{
    switch (levelId) {
    case 1: {
        // Level 1 has goblins
        auto* goblinSet = new AnimationSet();  // or use a member/pool
        sf::Texture& tex = levelRes.textures.get((int)Cfg::Textures::GoblinSheet);
        initGoblinAnimations(*goblinSet, tex);
        animSets[EntityType::Goblin] = goblinSet;
        break;
    }
    case 2: {
        // Level 2 might have goblins + orcs, etc.
        break;
    }
    default: break;
    }
}
