#include "PlayerManager.h"

int PlayerManager::getFreeSlot() const {
    for (int i = 0; i < 2; ++i)
        if (!slots[i].connected) return i;
    return -1;
}

bool PlayerManager::isConnected(int idx) const {
    return idx >= 0 && idx < 2 && slots[idx].connected;
}

uint32_t PlayerManager::getEntityId(int idx) const {
    return (idx >= 0 && idx < 2) ? playerEntityId[idx] : 0xFFFFFFFF;
}