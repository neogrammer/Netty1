#include "Level.h"

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