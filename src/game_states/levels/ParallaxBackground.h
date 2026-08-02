// ParallaxBackground.h
#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <res/SceneResources.h>
#include <stdexcept>
#include <memory>

class ParallaxBackground {
public:
    struct Layer {
        std::unique_ptr<sf::Sprite> sprite{ nullptr };
        float parallaxFactor{ 1.0f };
        bool tileX{ false };
    };

    void setGroundOffset(const sf::Vector2f& offset) { groundOffset = offset; }

    // Set the required far background layer (only one)
    void setFarLayer(SceneResources& res, int textureId, float factor, bool tileX = false);

    // Add a mid layer (0‑5 allowed). Returns false if max reached.
    bool addMidLayer(SceneResources& res, int textureId, float factor, bool tileX = false);

    // Set the required ground layer. Also defines the walkable polygon.
    void setGroundLayer(SceneResources& res, int textureId,
        const std::vector<sf::Vector2f>& walkablePoly,
        bool tileX = false);

    // Add a foreground layer (0‑2 allowed). Returns false if max reached.
    bool addForegroundLayer(SceneResources& res, int textureId, float factor, bool tileX = false);

    // Retrieve the walkable polygon (in world coordinates)
    const std::vector<sf::Vector2f>& getWalkablePolygon() const { return walkablePolygon; }

    // Draw all layers in the correct order
    void draw(sf::RenderWindow& window, const sf::Vector2f& cameraOffset);

    
    void setSpawnX(float x) { spawnX = x; }
private:
    Layer farLayer;
    std::vector<Layer> midLayers;
    Layer groundLayer;
    float spawnX = 0.f;
    std::vector<Layer> foregroundLayers;
    std::vector<sf::Vector2f> walkablePolygon;
    sf::Vector2f groundOffset{ 0.f, 0.f };  
    // Helper: draw one layer with optional horizontal tiling
    void drawLayer(sf::RenderWindow& window, Layer& layer, const sf::Vector2f& cameraOffset);
    void drawLayer(sf::RenderWindow& window, Layer& layer,
        const sf::Vector2f& cameraOffset, const sf::Vector2f& extraOffset);
    static constexpr int TILE_COUNT = 3;   // number of texture repeats for tiling
    static constexpr int MAX_MID = 5;
    static constexpr int MAX_FG = 2;
};