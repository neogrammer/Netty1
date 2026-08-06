// PlayState.cpp
#include "PlayState.h"
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <res/Cfg.h>
#include <cmath>
#include <entities/animation/enemies/GoblinAnimations.h>
#include <network/NetTypes.h>

PlayState::PlayState(sf::RenderWindow* win, ClientContext& ctx, AnimationSet& playerAnimSet)
    : window(win), context(ctx)
{
    entityAnimSets[EntityType::Player] = &playerAnimSet;
}


void PlayState::enter() {
    interpolator.reset();
    entities.clear();

    // Reset context HUD state
    context.player1Connected = false;
    context.player2Connected = false;
    context.player1Health = 100;
    context.player1MaxHealth = 100;
    context.player2Health = 100;
    context.player2MaxHealth = 100;

    loadLevel(context.currentLevel);

    // ... background setup unchanged ...
    background.setFarLayer(levelRes, (int)Cfg::Textures::L1_BgFar, 0.01f, true);
    background.addMidLayer(levelRes, (int)Cfg::Textures::L1_BgMid, 0.05f, true);
    background.addMidLayer(levelRes, (int)Cfg::Textures::L1_BgNear, 0.4f, true);
    background.addForegroundLayer(levelRes, (int)Cfg::Textures::L1_Foreground, 1.3f, true);
    std::vector<sf::Vector2f> groundPoly = {
        { 0, 702 }, { 18000, 702 }, { 18000, 798 }, { 0, 798 }
    };
    background.setGroundLayer(levelRes, (int)Cfg::Textures::L1_Ground, groundPoly, true);
    // Set own connection
    if (context.playerId == 0) {
        context.player1Connected = true;
        context.player1Health = 100;
        context.player1MaxHealth = 100;
    }
    else {
        context.player2Connected = true;
        context.player2Health = 100;
        context.player2MaxHealth = 100;
    }

   

    sf::Packet p;
    p << NetMsgType::PlayerReady;
    context.tcpSocket->send(p);

    printf("[PlayState] Sent PlayerReady to server\n");
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
            printf("[Client] Spawned entity %u type=%d animSet=%p\n",
                msg.entityId, (int)msg.entityType, ent.animSet);
            ent.currentAnim = static_cast<AnimType>(msg.animation);
            ent.animStartTick = msg.animStartTick;
            entities[msg.entityId] = std::move(ent);
        }
        else {
            printf("[Client] FAILED to spawn entity %u type=%d\n", msg.entityId, (int)msg.entityType);
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
                ent.health = snapEnt->health;
            }
        }
    }

    // Update HUD health from entity data
    for (auto& [id, ent] : entities) {
        if (ent.entityType == EntityType::Player) {
            // Player 0 entity ID is always the lower one assigned by server
            // We need to know which entity maps to which HUD slot
            // Simplest: use the entity ID mapping from context
            if (id == context.myEntityId) {
                // Own health
                if (context.playerId == 0) {
                    context.player1Health = ent.health;
                }
                else {
                    context.player2Health = ent.health;
                }
            }
            else {
                // Other player's health
                if (context.playerId == 0) {
                    context.player2Health = ent.health;
                }
                else {
                    context.player1Health = ent.health;
                }
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

        // --- Debug: strike box (matches server getStrikeBox) ---
        if (id == context.myEntityId && ent.currentAnim >= AnimType::Attack1 && ent.currentAnim <= AnimType::Attack3) {
            int idx = static_cast<int>(ent.currentAnim) - static_cast<int>(AnimType::Attack1);

            float sbOffX[3] = { 170.f, 170.f, 170.f };
            float sbOffY[3] = { 10.f, 40.f, 85.f };
            float sbW[3] = { 80.f, 80.f, 80.f };
            float sbH[3] = { 160.f, 130.f, 80.f };

            uint8_t facing = 0;
            if (auto* snapEnt = interpolator.findEntity(id)) facing = snapEnt->flags & 1;

            // Same logic as server getStrikeBox()
            float bodyCenterX = ent.x + 96.f + 74.f / 2.f;  // hitbox offsetX + width/2
            float sbx, sby = ent.y + sbOffY[idx];

            if (facing == 1) {
                sbx = ent.x + sbOffX[idx];
            }
            else {
                float distFromCenter = sbOffX[idx] - 96.f - 74.f / 2.f;
                sbx = bodyCenterX - distFromCenter - sbW[idx];
            }

            sf::RectangleShape debugBox({ sbW[idx], sbH[idx] });
            debugBox.setPosition({ std::round(sbx - cameraOffset.x), std::round(sby - cameraOffset.y) });
            debugBox.setFillColor(sf::Color(255, 0, 0, 100));
            // After computing the animation frame...
            printf("[Client] Anim=%d, frame=%d, tick=%u\n",
                (int)ent.currentAnim, frameIdx, ent.animStartTick);
            window.draw(debugBox);
        }

        // --- Debug: body hitbox ---
        {
            // Player hitbox hardcoded for debug (match server: offset 48,42 size 37,40)
            float hbOffX = 96.f;
            float hbOffY = 84.f;
            float hbW = 74.f;
            float hbH = 80.f;

            sf::RectangleShape bodyBox({ hbW, hbH });
            bodyBox.setPosition({
                std::round(ent.x + hbOffX - cameraOffset.x),
                std::round(ent.y + hbOffY - cameraOffset.y)
                });
            bodyBox.setFillColor(sf::Color(0, 255, 0, 80));   // green, semi-transparent
            bodyBox.setOutlineColor(sf::Color(0, 255, 0, 120));
            bodyBox.setOutlineThickness(1.f);
            window.draw(bodyBox);
        }

        // --- Debug: feet position ---
        {
            
            sf::RectangleShape feetMarker({ 6.f, 4.f });
            float feetY = ent.y + 84.f + 80.f;  // offsetY + height
            feetMarker.setPosition({
                std::round(ent.x + 96.f + 37.f - cameraOffset.x),  // offX + width/2
                std::round(feetY - cameraOffset.y)
                });
            feetMarker.setFillColor(sf::Color(255, 255, 0, 200));  // yellow
            window.draw(feetMarker);
        }

        window.draw(*ent.sprite);

        // --- Overhead health bar (enemies only) ---
        if (ent.entityType != EntityType::Player && ent.maxHealth > 0) {
            float barWidth = 50.f;
            float barHeight = 5.f;
            float healthPercent = (float)ent.health / ent.maxHealth;

            sf::Vector2f barPos = {
                std::round(ent.x - cameraOffset.x + 96.f + 37.f - barWidth / 2.f),  // centered above hitbox
                std::round(ent.y - cameraOffset.y - 10.f)                           // above sprite
            };

            // Background (dark red)
            sf::RectangleShape bgBar({ barWidth, barHeight });
            bgBar.setPosition(barPos);
            bgBar.setFillColor(sf::Color(40, 0, 0));
            window.draw(bgBar);

            // Foreground (green → yellow → red)
            sf::Color barColor;
            if (healthPercent > 0.6f)      barColor = sf::Color(0, 200, 0);
            else if (healthPercent > 0.3f) barColor = sf::Color(200, 200, 0);
            else                           barColor = sf::Color(200, 0, 0);

            sf::RectangleShape fgBar({ barWidth * healthPercent, barHeight });
            fgBar.setPosition(barPos);
            fgBar.setFillColor(barColor);
            window.draw(fgBar);

			
        }

    }
    
    // draw damage indicators and other dialog in game messages here


    // after entities, draw foreground layers
    background.drawForeground(window, cameraOffset);

    // --- HUD Plaques ---
    {
        sf::Sprite decal(Cfg::textures.get((int)Cfg::Textures::Ui_HealthDecal));
        sf::Font& hudFont = Cfg::fonts.get((int)Cfg::Fonts::Bubbly);

        auto drawPlaque = [&](float x, const std::string& label, int health, int maxHealth, bool connected) {
            // Label above the decal
            sf::Text nameText(hudFont, connected ? label : "Waiting For Player", 22);
            nameText.setPosition({ x + 10.f, 8.f });
            nameText.setFillColor(connected ? sf::Color::White : sf::Color(255, 255, 100, 200));
            nameText.setOutlineThickness(2.f);
            nameText.setOutlineColor(sf::Color(0, 0, 0));
            window.draw(nameText);

            // Decal below the label
            decal.setPosition({ x, 8.f + 26.f });
            window.draw(decal);

            // Health numbers inside the decal
            if (connected) {
                std::string hpStr = std::to_string(health) + " / " + std::to_string(maxHealth);
                sf::Text hpText(hudFont, hpStr, 18);
                hpText.setPosition({ x + 73.f, 38.f + 10.f });
                hpText.setFillColor(sf::Color::White);
                hpText.setOutlineThickness(2.f);
                hpText.setOutlineColor(sf::Color(0, 0, 0));
                window.draw(hpText);
                
            }
            };

        drawPlaque(10.f, "Player 1", context.player1Health, context.player1MaxHealth, context.player1Connected);
        drawPlaque(300.f, "Player 2", context.player2Health, context.player2MaxHealth, context.player2Connected);
    }

    // display the frame to the window context
    window.display();

}


void PlayState::unloadCurrentLevel() {
    ownedAnimSets.clear();
    entityAnimSets.clear();
    entities.clear();
    levelRes.clear();
}


void PlayState::loadLevel(int levelId) {
    // Save player anim set pointers
    auto player0It = entityAnimSets.find(EntityType::Player);
    AnimationSet* player0Set = (player0It != entityAnimSets.end()) ? player0It->second : nullptr;

    // Clear level-specific sets
    ownedAnimSets.clear();
    entityAnimSets.clear();

    // Restore player
    if (player0Set) {
        entityAnimSets[EntityType::Player] = player0Set;
    }

    context.currentLevel = levelId;
    levelRes.loadForScene(levelId, true);
    setupEntityAnimSets(levelId);
    interpolator.reset();
}

void PlayState::setupEntityAnimSets(int levelId) {
    switch (levelId) {
    case 1: {
        auto goblinSet = std::make_unique<AnimationSet>();
        sf::Texture& tex = levelRes.textures.get((int)Cfg::Textures::GoblinSheet);
        initGoblinAnimations(*goblinSet, tex);
        entityAnimSets[EntityType::Goblin] = goblinSet.get();
        ownedAnimSets[EntityType::Goblin] = std::move(goblinSet);
        break;
    }
    }
}