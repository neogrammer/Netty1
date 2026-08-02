// PlayState.h
#pragma once
#include <game_states/GameState.h>
#include <network/NetTypes.h>
#include <entities/Entity.h>
#include <entities/animation/AnimationSet.h>
#include <res/SceneResources.h>
#include <network/client/ClientContext.h>
#include <SFML/Network.hpp>
#include <unordered_map>

#include <SFML/Graphics.hpp>
#include <vector>
#include <game_states/levels/ParallaxBackground.h>
#include <network/trans/SnapshotInterpolator.h>

#include <res/SceneResources.h>
#include <entities/animation/AnimationSet.h>

class PlayState : public IGameState {
public:
    PlayState(sf::RenderWindow* win, ClientContext& ctx,
        const std::unordered_map<EntityType, AnimationSet*>& animSets);
        
    void enter() override;
    void exit() override;
    void handleEvent(const sf::Event&) override;
    void update(sf::Time dt) override;
    void draw(sf::RenderWindow& window) override;

	
private:
    SnapshotInterpolator interpolator;
    sf::RenderWindow* window;
    ClientContext& context;
    ParallaxBackground background;  
    bool anchorSet = false;
    sf::Vector2f prevCameraCenter{ 0.f, 450.f };
    sf::Vector2f currCameraCenter{ 0.f, 450.f };

    const std::unordered_map<EntityType, AnimationSet*>& entityAnimSets;
    SceneResources levelRes;
    // Entity state
    std::unordered_map<uint32_t, ClientEntity> entities;
  
    const sf::Time tickDuration = sf::seconds(1.f / 60.f);


    // existing helpers (slightly modified to use context.myEntityId)
    void interpolateEntities(float renderTick);
};
