//#include "PlayState.h"
//#include <cstdio>
//#include <algorithm>
//#include <cstring>
//#include <res/Cfg.h>
//
//PlayState::PlayState(sf::RenderWindow* win, ClientContext& ctx,
//    const std::unordered_map<EntityType, AnimationSet*>& animSets)
//    : window(win), context(ctx), entityAnimSets(animSets) {}
//    
//
//void PlayState::enter() {
//    // Assets are already loaded by central TCP handler before switching
//    // Snapshot interpolation starts fresh
//    levelRes.loadForScene(context.currentLevel, true);
//    
//    // Build parallax background
//    background.setFarLayer(levelRes, (int)Cfg::Textures::L1_BgFar, 0.1f, true);
//    background.addMidLayer(levelRes, (int)Cfg::Textures::L1_BgMid_Far, 0.3f, true);
//    background.addMidLayer(levelRes, (int)Cfg::Textures::L1_BgMid_Mid, 0.5f, true);
//    background.addMidLayer(levelRes, (int)Cfg::Textures::L1_BgMid_Near, 0.7f, true);
//
//    // Ground with a simple walkable rectangle (adjust to your level)
//    std::vector<sf::Vector2f> groundPoly = {
//        { 0, 600 }, { 18000, 600 }, { 18000, 900 }, { 0, 900 }   // example
//    };
//    background.setGroundLayer(levelRes, (int)Cfg::Textures::L1_Foreground, groundPoly, true);
//    // After background layers are set up
//    //float floorWorldY = 600.f;   // change to your desired floor Y
//    //background.setGroundOffset({ 0.f, floorWorldY - 900.f });
//
//    background.setFloorY(500.f);   // or whatever Y your floor is at
//    background.setSpawnX(0.f);
//    // Foreground layers (none for now)
//    
//    snapState.hasPrev = false;
//    interpClock.restart();
//    printf("[PlayState] Entered level %d\n", context.currentLevel);
//
//
//}
//
//void PlayState::exit() {
//   // printf("[PlayState] Exiting\n");
//    entities.clear();
//}
//
//void PlayState::handleEvent(const sf::Event& event) {
//    // Not used for gameplay input
//}
//
//
//
//void PlayState::update(sf::Time dt) {
//
//    // Process pending spawns
//    for (auto& msg : context.pendingSpawns) {
//        auto [ok, ent] = createClientEntity(entityAnimSets, msg.entityType, msg.x, msg.y);
//        if (ok) {
//            ent.currentAnim = static_cast<AnimType>(msg.animation);
//            ent.animStartTick = msg.animStartTick;   // <-- set from spawn
//            entities[msg.entityId] = std::move(ent);
//        }
//    }
//    context.pendingSpawns.clear();
//
//    // Process pending destroys
//    for (auto& msg : context.pendingDestroys) {
//        entities.erase(msg.entityId);
//        if (context.myEntityId == msg.entityId)
//            context.myEntityId = 0xFFFFFFFF;
//    }
//    context.pendingDestroys.clear();
//
//    // Process snapshot if new one arrived
//    if (context.hasSnapshot) {
//        snapState.prev = std::move(snapState.curr);
//        context.latestSnapshot >> snapState.curr;
//        printf("[Client %d] Got snapshot frame %u with %zu entities\n",
//            context.playerId, snapState.curr.frameNumber, snapState.curr.entities.size());
//        if (!snapState.curr.entities.empty()) {
//            for (auto& e : snapState.curr.entities) {
//                printf("   entity %u: x_quant=%d y_quant=%d anim=%u tick=%u\n",
//                    e.entityId, e.x_quant, e.y_quant, e.animation, e.animStartTick);
//            }
//        }
//        snapState.lastSnapTime = interpClock.getElapsedTime();
//        snapState.hasPrev = true;
//        context.hasSnapshot = false;
//    }
//
//
//    // Input sending is handled centrally in run_client, so nothing here.
//    // Entities are updated by snapshots received centrally and stored in our entities map.
//    // Interpolation:
//    currentRenderTick = 0.f;
//    if (snapState.hasPrev) {
//        sf::Time now = interpClock.getElapsedTime();
//        float t = ((now - snapState.lastSnapTime).asSeconds()) / tickDuration.asSeconds();
//        t = std::min(t, 1.0f);
//        uint32_t prevTick = snapState.prev.frameNumber;
//        uint32_t currTick = snapState.curr.frameNumber;
//        currentRenderTick = prevTick + t * (currTick - prevTick);
//        interpolateEntities(currentRenderTick);
//    }
//}
//
//void PlayState::draw(sf::RenderWindow& window) {
//  
//    window.clear();
//
//	// draw parallax background here
//    // Compute camera offset (same as before)
//    sf::Vector2f cameraCenter;
//    if (snapState.hasPrev) {
//        cameraCenter = { unquantise(snapState.curr.camX_quant),
//                         unquantise(snapState.curr.camY_quant) };
//    }
//    else {
//        // fallback to local player position (only used before first snapshot)
//        if (context.myEntityId != 0xFFFFFFFF) {
//            auto it = entities.find(context.myEntityId);
//            if (it != entities.end())
//                cameraCenter = { it->second.x, it->second.y };
//        }
//    }
//    sf::Vector2f cameraOffset = cameraCenter - sf::Vector2f(800.f, 450.f);
//    background.draw(window, cameraOffset);
//
//    // draw the game entities
//    for (auto& [id, ent] : entities) {
//        if (!ent.sprite) continue;
//
//        int frameIdx = 0;
//        if (ent.animSet) {
//            AnimationSet& anim = *ent.animSet;
//            AnimType cur = ent.currentAnim;
//
//            // Calculate animation frame
//            float elapsedTicks = std::max(0.f, currentRenderTick - ent.animStartTick);
//            float timeSec = elapsedTicks * tickDuration.asSeconds();
//            float durPerFrame = anim.animDurations[cur];
//            int totalFrames = anim.frameCounts[cur];
//            if (totalFrames > 0) {
//                int rawFrame = static_cast<int>(timeSec / durPerFrame);
//                frameIdx = anim.loops[cur] ? (rawFrame % totalFrames)
//                    : std::min(rawFrame, totalFrames - 1);
//            }
//
//            // Determine facing (flags bit0)
//            uint8_t facing = 0;
//            // Look up the entity in the current snapshot (if available) to get flags
//            if (snapState.hasPrev) {
//                auto snapIt = std::find_if(snapState.curr.entities.begin(),
//                    snapState.curr.entities.end(),
//                    [&](const EntitySnapshot& s) { return s.entityId == id; });
//                if (snapIt != snapState.curr.entities.end())
//                    facing = snapIt->flags & 1;
//            }
//
//            // Apply the correct texture and texture rect
//            if (anim.animMap.count(cur)) {
//                sf::Texture* tex = anim.animMap[cur];
//                if (tex) ent.sprite->setTexture(*tex);
//                if (anim.animRects.count(cur) &&
//                    anim.animRects[cur][facing].size() > static_cast<size_t>(frameIdx)) {
//                    sf::IntRect rect = anim.animRects[cur][facing][frameIdx];
//                    ent.sprite->setTextureRect(rect);
//                }
//            }
//        }
//
//        ent.sprite->setPosition({ ent.x, ent.y });
//        window.draw(*ent.sprite);
//    }
//	// draw any info overlays like damage
//	// draw foreground elements like trees, walls, etc
//	// draw UI elements like health bars, score, etc.
//    window.display();
//}
//
//// ---------- Private helpers (identical to old code) ----------
//void PlayState::interpolateEntities(float renderTick) {
//
//    printf("[Client %d] Interpolating with currentRenderTick=%.2f\n",
//        context.playerId, renderTick);
//
//    if (!snapState.hasPrev) return;
//
//    float t = 0.f;
//    if (snapState.curr.frameNumber != snapState.prev.frameNumber) {
//        t = (renderTick - snapState.prev.frameNumber) /
//            (float)(snapState.curr.frameNumber - snapState.prev.frameNumber);
//    }
//    t = std::min(t, 1.0f);
//
//    for (const auto& snapEnt : snapState.curr.entities) {
//        auto it = entities.find(snapEnt.entityId);
//        if (it == entities.end()) continue;
//
//        float x = unquantise(snapEnt.x_quant);
//        float y = unquantise(snapEnt.y_quant);
//        auto prevIt = std::find_if(snapState.prev.entities.begin(), snapState.prev.entities.end(),
//            [&](const EntitySnapshot& s) { return s.entityId == snapEnt.entityId; });
//        if (prevIt != snapState.prev.entities.end()) {
//            float prevX = unquantise(prevIt->x_quant);
//            float prevY = unquantise(prevIt->y_quant);
//            x = prevX + t * (x - prevX);
//            y = prevY + t * (y - prevY);
//        }
//        it->second.x = x;
//        it->second.y = y;
//        it->second.animStartTick = snapEnt.animStartTick;
//    }
//}

#include "PlayState.h"
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <res/Cfg.h>
#include <cmath>

PlayState::PlayState(sf::RenderWindow* win, ClientContext& ctx,
    const std::unordered_map<EntityType, AnimationSet*>& animSets)
    : window(win), context(ctx), entityAnimSets(animSets) {}

void PlayState::enter() {
    levelRes.loadForScene(context.currentLevel, true);

    background.setFarLayer(levelRes, (int)Cfg::Textures::L1_BgFar, 0.1f, true);
    background.addMidLayer(levelRes, (int)Cfg::Textures::L1_BgMid_Far, 0.3f, true);
    background.addMidLayer(levelRes, (int)Cfg::Textures::L1_BgMid_Mid, 0.5f, true);
    background.addMidLayer(levelRes, (int)Cfg::Textures::L1_BgMid_Near, 0.7f, true);

    std::vector<sf::Vector2f> groundPoly = {
        { 0, 600 }, { 18000, 600 }, { 18000, 900 }, { 0, 900 }
    };
    background.setGroundLayer(levelRes, (int)Cfg::Textures::L1_Foreground, groundPoly, true);
    
    
   // currCameraCenter = { 100.f, 450.f };
  //  prevCameraCenter = currCameraCenter;
    snapState.hasPrev = false;
    interpClock.restart();
    printf("[PlayState] Entered level %d\n", context.currentLevel);
}

void PlayState::exit() {
    entities.clear();
}

void PlayState::handleEvent(const sf::Event& event) {
    // Not used for gameplay input
}
//
//void PlayState::update(sf::Time dt) {
//    // Process pending spawns
//    for (auto& msg : context.pendingSpawns) {
//        auto [ok, ent] = createClientEntity(entityAnimSets, msg.entityType, msg.x, msg.y);
//        if (ok) {
//            ent.currentAnim = static_cast<AnimType>(msg.animation);
//            ent.animStartTick = msg.animStartTick;
//            entities[msg.entityId] = std::move(ent);
//        }
//    }
//    context.pendingSpawns.clear();
//
//    // Process pending destroys
//    for (auto& msg : context.pendingDestroys) {
//        entities.erase(msg.entityId);
//        if (context.myEntityId == msg.entityId)
//            context.myEntityId = 0xFFFFFFFF;
//    }
//    context.pendingDestroys.clear();
//
//    // Process snapshot if new one arrived
//// Process snapshot if new one arrived
//    if (context.hasSnapshot) {
//        snapState.prev = std::move(snapState.curr);
//        context.latestSnapshot >> snapState.curr;
//        snapState.lastSnapTime = interpClock.getElapsedTime();
//        snapState.hasPrev = true;
//        context.hasSnapshot = false;
//
//        // Update camera center AFTER deserializing the new snapshot
//        //prevCameraCenter = currCameraCenter;
//        //currCameraCenter = { unquantise(snapState.curr.camX_quant), 450.f };
//    }
//
//    // Interpolation
//// Interpolation (runs every frame, even if no new snapshot)
//    currentRenderTick = 0.f;
//    if (snapState.hasPrev) {
//        sf::Time now = interpClock.getElapsedTime();
//        float elapsed = (now - snapState.lastSnapTime).asSeconds();
//        // Predict how many ticks have passed since the last snapshot
//        float ticksElapsed = elapsed / tickDuration.asSeconds();
//        // Cap at 3 ticks to avoid huge jumps after a lag spike
//        if (ticksElapsed > 3.f) ticksElapsed = 3.f;
//        currentRenderTick = snapState.curr.frameNumber + ticksElapsed;
//        interpolateEntities(currentRenderTick);
//    }
//}
//
//void PlayState::draw(sf::RenderWindow& window) {
//    window.clear();
//
//    // Camera from snapshot
//    //sf::Vector2f cameraCenter;
//    //if (snapState.hasPrev) {
//    //    cameraCenter = { unquantise(snapState.curr.camX_quant),
//    //                     450.f };
//    //}
//    //else {
//    //    if (context.myEntityId != 0xFFFFFFFF) {
//    //        auto it = entities.find(context.myEntityId);
//    //        if (it != entities.end())
//    //            cameraCenter = { it->second.x, 450.f };
//    //    }
//    //}
//   
//    // Smooth camera
//    //sf::Vector2f cameraCenter = currCameraCenter;
//    //if (snapState.hasPrev) {
//    //    // Use the same t as entity interpolation
//    //    sf::Time now = interpClock.getElapsedTime();
//    //    float tCam = ((now - snapState.lastSnapTime).asSeconds()) / tickDuration.asSeconds();
//    //    tCam = std::min(tCam, 1.0f);
//    //    cameraCenter = prevCameraCenter + (currCameraCenter - prevCameraCenter) * tCam;
//    //}
//    //sf::Vector2f cameraOffset = cameraCenter - sf::Vector2f(800.f, 450.f);
//    // Extrapolate camera offset between snapshots
//    sf::Vector2f cameraCenter;
//    if (snapState.hasPrev) {
//        float camX = unquantise(snapState.curr.camX_quant);
//        float camXPrev = unquantise(snapState.prev.camX_quant);
//        // How many ticks passed since last snapshot
//        sf::Time now = interpClock.getElapsedTime();
//        float elapsed = (now - snapState.lastSnapTime).asSeconds();
//        float ticksElapsed = elapsed / tickDuration.asSeconds();
//        if (ticksElapsed > 3.f) ticksElapsed = 3.f;
//        // Extrapolate camera forward
//        float camXExtrap = camX + (camX - camXPrev) * ticksElapsed;
//        cameraCenter = { camXExtrap, 450.f };
//    }
//    else {
//        cameraCenter = { 100.f, 450.f };   // fallback spawn position
//    }
//    sf::Vector2f cameraOffset = cameraCenter - sf::Vector2f(800.f, 450.f);
//
//    //sf::Vector2f cameraCenter;
//    //if (snapState.hasPrev) {
//    //    cameraCenter = { unquantise(snapState.curr.camX_quant), 450.f };
//    //}
//    //else {
//    //    if (context.myEntityId != 0xFFFFFFFF) {
//    //        auto it = entities.find(context.myEntityId);
//    //        if (it != entities.end())
//    //            cameraCenter = { it->second.x, 450.f };
//    //    }
//    //}
//    //sf::Vector2f cameraOffset = cameraCenter - sf::Vector2f(800.f, 450.f);
//
//    background.draw(window, cameraOffset);
//
//    // Draw entities
//    for (auto& [id, ent] : entities) {
//        if (!ent.sprite) continue;
//
//        int frameIdx = 0;
//        if (ent.animSet) {
//            AnimationSet& anim = *ent.animSet;
//            AnimType cur = ent.currentAnim;
//
//            float elapsedTicks = std::max(0.f, currentRenderTick - ent.animStartTick);
//            float timeSec = elapsedTicks * tickDuration.asSeconds();
//            float durPerFrame = anim.animDurations[cur];
//            int totalFrames = anim.frameCounts[cur];
//            if (totalFrames > 0) {
//                int rawFrame = static_cast<int>(timeSec / durPerFrame);
//                frameIdx = anim.loops[cur] ? (rawFrame % totalFrames) : std::min(rawFrame, totalFrames - 1);
//            }
//
//            uint8_t facing = 0;
//            if (snapState.hasPrev) {
//                auto snapIt = std::find_if(snapState.curr.entities.begin(), snapState.curr.entities.end(),
//                    [&](const EntitySnapshot& s) { return s.entityId == id; });
//                if (snapIt != snapState.curr.entities.end())
//                    facing = snapIt->flags & 1;
//            }
//
//            if (anim.animMap.count(cur)) {
//                sf::Texture* tex = anim.animMap[cur];
//                if (tex) ent.sprite->setTexture(*tex);
//                if (anim.animRects.count(cur) &&
//                    anim.animRects[cur][facing].size() > static_cast<size_t>(frameIdx)) {
//                    sf::IntRect rect = anim.animRects[cur][facing][frameIdx];
//                    ent.sprite->setTextureRect(rect);
//                }
//            }
//        }
//
//        ent.sprite->setPosition({std::round(ent.x - cameraOffset.x), std::round(ent.y - cameraOffset.y) });
//        window.draw(*ent.sprite);
//    }
//
//    window.display();
//}
//
//void PlayState::interpolateEntities(float renderTick) {
//    if (!snapState.hasPrev) return;
//
//    float t = 0.f;
//    if (snapState.curr.frameNumber != snapState.prev.frameNumber) {
//        t = (renderTick - snapState.prev.frameNumber) /
//            (float)(snapState.curr.frameNumber - snapState.prev.frameNumber);
//    }
//    t = std::min(t, 1.0f);
//
//    for (const auto& snapEnt : snapState.curr.entities) {
//        auto it = entities.find(snapEnt.entityId);
//        if (it == entities.end()) continue;
//
//        float x = unquantise(snapEnt.x_quant);
//        float y = unquantise(snapEnt.y_quant);
//
//        auto prevIt = std::find_if(snapState.prev.entities.begin(), snapState.prev.entities.end(),
//            [&](const EntitySnapshot& s) { return s.entityId == snapEnt.entityId; });
//        if (prevIt != snapState.prev.entities.end()) {
//            float prevX = unquantise(prevIt->x_quant);
//            float prevY = unquantise(prevIt->y_quant);
//            x = prevX + t * (x - prevX);
//            y = prevY + t * (y - prevY);
//        }
//
//        it->second.x = x;
//        it->second.y = y;
//        it->second.animStartTick = snapEnt.animStartTick;
//    }
//}

void PlayState::update(sf::Time dt) {
    // Process pending spawns
    for (auto& msg : context.pendingSpawns) {
        auto [ok, ent] = createClientEntity(entityAnimSets, msg.entityType, msg.x, msg.y);
        if (ok) {
            ent.currentAnim = static_cast<AnimType>(msg.animation);
            ent.animStartTick = msg.animStartTick;
            entities[msg.entityId] = std::move(ent);
        }
    }
    context.pendingSpawns.clear();

    // Process pending destroys
    for (auto& msg : context.pendingDestroys) {
        entities.erase(msg.entityId);
        if (context.myEntityId == msg.entityId)
            context.myEntityId = 0xFFFFFFFF;
    }
    context.pendingDestroys.clear();

    // Process snapshot if new one arrived
    if (context.hasSnapshot) {
        snapState.prev = std::move(snapState.curr);
        context.latestSnapshot >> snapState.curr;
        snapState.lastSnapTime = interpClock.getElapsedTime();
        snapState.hasPrev = true;
        context.hasSnapshot = false;
    }

    // Interpolation
    currentRenderTick = 0.f;
    if (snapState.hasPrev) {
        sf::Time now = interpClock.getElapsedTime();
        float t = ((now - snapState.lastSnapTime).asSeconds()) / tickDuration.asSeconds();
        t = std::min(t, 1.0f);
        currentRenderTick = snapState.prev.frameNumber + t * (snapState.curr.frameNumber - snapState.prev.frameNumber);
        interpolateEntities(currentRenderTick);
    }
}

void PlayState::draw(sf::RenderWindow& window) {
    window.clear();

    sf::Vector2f cameraCenter;
    if (snapState.hasPrev) {
        float camX = unquantise(snapState.curr.camX_quant);
        float camXPrev = unquantise(snapState.prev.camX_quant);
        // Use the same t as entity interpolation
        sf::Time now = interpClock.getElapsedTime();
        float tCam = ((now - snapState.lastSnapTime).asSeconds() + tickDuration.asSeconds()) / tickDuration.asSeconds();
        //float tCam = ((now - snapState.lastSnapTime).asSeconds()) / tickDuration.asSeconds();
        tCam = std::min(tCam, 1.0f);
        float camXInterp = camXPrev + tCam * (camX - camXPrev);
        cameraCenter = { camXInterp, 450.f };
    }
    else {
        cameraCenter = { 100.f, 450.f };
    }
    sf::Vector2f cameraOffset = cameraCenter - sf::Vector2f(800.f, 450.f);

    background.draw(window, cameraOffset);

    // Draw entities
    for (auto& [id, ent] : entities) {
        if (!ent.sprite) continue;

        int frameIdx = 0;
        if (ent.animSet) {
            AnimationSet& anim = *ent.animSet;
            AnimType cur = ent.currentAnim;

            float elapsedTicks = std::max(0.f, currentRenderTick - ent.animStartTick);
            float timeSec = elapsedTicks * tickDuration.asSeconds();
            float durPerFrame = anim.animDurations[cur];
            int totalFrames = anim.frameCounts[cur];
            if (totalFrames > 0) {
                int rawFrame = static_cast<int>(timeSec / durPerFrame);
                frameIdx = anim.loops[cur] ? (rawFrame % totalFrames) : std::min(rawFrame, totalFrames - 1);
            }

            uint8_t facing = 0;
            if (snapState.hasPrev) {
                auto snapIt = std::find_if(snapState.curr.entities.begin(), snapState.curr.entities.end(),
                    [&](const EntitySnapshot& s) { return s.entityId == id; });
                if (snapIt != snapState.curr.entities.end())
                    facing = snapIt->flags & 1;
            }

            if (anim.animMap.count(cur)) {
                sf::Texture* tex = anim.animMap[cur];
                if (tex) ent.sprite->setTexture(*tex);
                if (anim.animRects.count(cur) &&
                    anim.animRects[cur][facing].size() > static_cast<size_t>(frameIdx)) {
                    sf::IntRect rect = anim.animRects[cur][facing][frameIdx];
                    ent.sprite->setTextureRect(rect);
                }
            }
        }

        ent.sprite->setPosition({ std::round(ent.x - cameraOffset.x), std::round(ent.y - cameraOffset.y) });
        window.draw(*ent.sprite);
    }

    window.display();
}

void PlayState::interpolateEntities(float renderTick) {
    if (!snapState.hasPrev) return;

    float t = 0.f;
    if (snapState.curr.frameNumber != snapState.prev.frameNumber) {
        t = (renderTick - snapState.prev.frameNumber) /
            (float)(snapState.curr.frameNumber - snapState.prev.frameNumber);
    }
    t = std::min(t, 1.0f);

    for (const auto& snapEnt : snapState.curr.entities) {
        auto it = entities.find(snapEnt.entityId);
        if (it == entities.end()) continue;

        float x = unquantise(snapEnt.x_quant);
        float y = unquantise(snapEnt.y_quant);

        auto prevIt = std::find_if(snapState.prev.entities.begin(), snapState.prev.entities.end(),
            [&](const EntitySnapshot& s) { return s.entityId == snapEnt.entityId; });
        if (prevIt != snapState.prev.entities.end()) {
            float prevX = unquantise(prevIt->x_quant);
            float prevY = unquantise(prevIt->y_quant);
            x = prevX + t * (x - prevX);
            y = prevY + t * (y - prevY);
        }

        it->second.x = x;
        it->second.y = y;
        it->second.animStartTick = snapEnt.animStartTick;
    }
}

//void PlayState::interpolateEntities(float renderTick) {
//    if (!snapState.hasPrev) return;
//
//    float t = 0.f;
//    if (snapState.curr.frameNumber != snapState.prev.frameNumber) {
//        t = (renderTick - snapState.prev.frameNumber) /
//            (float)(snapState.curr.frameNumber - snapState.prev.frameNumber);
//    }
//    t = std::min(t, 1.0f);
//
//    for (const auto& snapEnt : snapState.curr.entities) {
//        auto it = entities.find(snapEnt.entityId);
//        if (it == entities.end()) continue;
//
//        float x = unquantise(snapEnt.x_quant);
//        float y = unquantise(snapEnt.y_quant);
//
//        // Only interpolate remote players; snap local player directly
//        if (snapEnt.entityId != context.myEntityId) {
//            auto prevIt = std::find_if(snapState.prev.entities.begin(), snapState.prev.entities.end(),
//                [&](const EntitySnapshot& s) { return s.entityId == snapEnt.entityId; });
//            if (prevIt != snapState.prev.entities.end()) {
//                float prevX = unquantise(prevIt->x_quant);
//                float prevY = unquantise(prevIt->y_quant);
//                x = prevX + t * (x - prevX);
//                y = prevY + t * (y - prevY);
//            }
//        }
//
//        it->second.x = x;
//        it->second.y = y;
//        it->second.animStartTick = snapEnt.animStartTick;
//    }
//}

//void PlayState::interpolateEntities(float renderTick) {
//    if (!snapState.hasPrev) return;
//
//    float t = 0.f;
//    if (snapState.curr.frameNumber != snapState.prev.frameNumber) {
//        t = (renderTick - snapState.prev.frameNumber) /
//            (float)(snapState.curr.frameNumber - snapState.prev.frameNumber);
//    }
//    t = std::min(t, 1.0f);
//
//    for (const auto& snapEnt : snapState.curr.entities) {
//        auto it = entities.find(snapEnt.entityId);
//        if (it == entities.end()) continue;
//
//        float x = unquantise(snapEnt.x_quant);
//        float y = unquantise(snapEnt.y_quant);
//        auto prevIt = std::find_if(snapState.prev.entities.begin(), snapState.prev.entities.end(),
//            [&](const EntitySnapshot& s) { return s.entityId == snapEnt.entityId; });
//        if (prevIt != snapState.prev.entities.end()) {
//            float prevX = unquantise(prevIt->x_quant);
//            float prevY = unquantise(prevIt->y_quant);
//            x = prevX + t * (x - prevX);
//            y = prevY + t * (y - prevY);
//        }
//        it->second.x = x;
//        it->second.y = y;
//        it->second.animStartTick = snapEnt.animStartTick;
//    }
//}