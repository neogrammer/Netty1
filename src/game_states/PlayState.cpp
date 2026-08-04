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

    background.setFarLayer(levelRes, (int)Cfg::Textures::L1_BgFar, 0.01f, true);
    background.addMidLayer(levelRes, (int)Cfg::Textures::L1_BgMid, 0.05f, true);
    background.addMidLayer(levelRes, (int)Cfg::Textures::L1_BgNear, 0.4f, true);
    background.addForegroundLayer(levelRes, (int)Cfg::Textures::L1_Foreground, 1.3f, true);
    std::vector<sf::Vector2f> groundPoly = {
        { 0, 702 }, { 18000, 702 }, { 18000, 798 }, { 0, 798 }
    };
    background.setGroundLayer(levelRes, (int)Cfg::Textures::L1_Ground, groundPoly, true);
    
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

    // Feed snapshot to interpolator
    if (context.hasSnapshot) {
        FrameSnapshot snap;
        context.latestSnapshot >> snap;
        interpolator.pushSnapshot(snap);
        context.hasSnapshot = false;
    }

    // Update interpolation
    if (interpolator.update()) {
        // Update all entity positions from interpolator
        for (auto& [id, ent] : entities) {
            float x, y;
            if (interpolator.getEntityPosition(id, x, y)) {
                ent.x = x;
                ent.y = y;
                if (id == context.myEntityId && ent.y != lastPrintedY) {
                    printf("[Client] My entity Y: %.1f\n", ent.y);
                    lastPrintedY = ent.y;
                }
            }
            // Update animStartTick from snapshot
            if (auto* snapEnt = interpolator.findEntity(id)) {
                ent.animStartTick = snapEnt->animStartTick; 
                ent.currentAnim = static_cast<AnimType>(snapEnt->animation);
            }
        }
    }
}


void PlayState::draw(sf::RenderWindow& window) {
    window.clear();

    sf::Vector2f cameraCenter = interpolator.getCameraPosition();
    sf::Vector2f cameraOffset = cameraCenter - sf::Vector2f(800.f, 450.f);


    background.draw(window, cameraOffset);

    // Draw entities
    for (auto& [id, ent] : entities) {
        if (!ent.sprite) continue;

        int frameIdx = 0;
        if (ent.animSet) {
            AnimationSet& anim = *ent.animSet;
            AnimType cur = ent.currentAnim;

            float renderTick = interpolator.getRenderTick();
            float elapsedTicks = std::max(0.f, renderTick - ent.animStartTick);
            float timeSec = elapsedTicks * tickDuration.asSeconds();
            float durPerFrame = anim.animDurations[cur];
            int totalFrames = anim.frameCounts[cur];
            if (totalFrames > 0) {
                int rawFrame = static_cast<int>(timeSec / durPerFrame);
                frameIdx = anim.loops[cur] ? (rawFrame % totalFrames) : std::min(rawFrame, totalFrames - 1);
            }

            uint8_t facing = 0;
            if (auto* snapEnt = interpolator.findEntity(id)) {
                facing = snapEnt->flags & 1;
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
    
    // draw damage indicators and other dialog in game messages here


	// after entities, draw foreground layers
	background.drawForeground(window, cameraOffset);

    // Now draw UI

    // display the frame to the window context
    window.display();
}
