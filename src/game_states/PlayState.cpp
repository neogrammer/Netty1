// PlayState.cpp
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

    // --- Compute interpolation factor ONCE ---
    if (snapState.hasPrev) {
        sf::Time now = interpClock.getElapsedTime();
        float rawT = (now - snapState.lastSnapTime).asSeconds() / tickDuration.asSeconds();
        snapState.interpT = std::clamp(rawT, 0.0f, 1.0f);

        // Compute the continuous render tick for animation timing
        currentRenderTick = snapState.prev.frameNumber
            + snapState.interpT * (snapState.curr.frameNumber - snapState.prev.frameNumber);

        // Interpolate all entities using the same t
        interpolateEntities(snapState.interpT);
    }
}

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
//    if (context.hasSnapshot) {
//        snapState.prev = std::move(snapState.curr);
//        context.latestSnapshot >> snapState.curr;
//        snapState.lastSnapTime = interpClock.getElapsedTime();
//        snapState.hasPrev = true;
//        context.hasSnapshot = false;
//    }
//
//    // Interpolation
//    currentRenderTick = 0.f;
//    if (snapState.hasPrev) {
//        sf::Time now = interpClock.getElapsedTime();
//        float t = ((now - snapState.lastSnapTime).asSeconds()) / tickDuration.asSeconds();
//        t = std::min(t, 1.0f);
//        currentRenderTick = snapState.prev.frameNumber + t * (snapState.curr.frameNumber - snapState.prev.frameNumber);
//        interpolateEntities(currentRenderTick);
//    }
//}

void PlayState::draw(sf::RenderWindow& window) {
    window.clear();

    sf::Vector2f cameraCenter;
    if (snapState.hasPrev) {
        float camX = unquantise(snapState.curr.camX_quant);
        float camXPrev = unquantise(snapState.prev.camX_quant);
        float camY = unquantise(snapState.curr.camY_quant);
        float camYPrev = unquantise(snapState.prev.camY_quant);

        // Use the SAME interpolation factor as entities
        float camXInterp = camXPrev + snapState.interpT * (camX - camXPrev);
        float camYInterp = camYPrev + snapState.interpT * (camY - camYPrev);

        cameraCenter = { camXInterp, 450.f };  // Y is fixed for now, but future-proofed
    }
    else {
        cameraCenter = { 100.f, 450.f };
    }

    //sf::Vector2f cameraCenter;
    //if (snapState.hasPrev) {
    //    float camX = unquantise(snapState.curr.camX_quant);
    //    float camXPrev = unquantise(snapState.prev.camX_quant);
    //    // Use the same t as entity interpolation
    //    sf::Time now = interpClock.getElapsedTime();
    //    float tCam = ((now - snapState.lastSnapTime).asSeconds() + tickDuration.asSeconds()) / tickDuration.asSeconds();
    //    //float tCam = ((now - snapState.lastSnapTime).asSeconds()) / tickDuration.asSeconds();
    //    tCam = std::min(tCam, 1.0f);
    //    float camXInterp = camXPrev + tCam * (camX - camXPrev);
    //    cameraCenter = { camXInterp, 450.f };
    //}
    //else {
    //    cameraCenter = { 100.f, 450.f };
    //}
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

void PlayState::interpolateEntities(float t) {
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