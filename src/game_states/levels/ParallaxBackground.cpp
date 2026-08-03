// ParallaxBackground.cpp
#include "ParallaxBackground.h"
#include <cmath>

// ---------- helper to set a layer's texture ----------
static void setLayerTexture(ParallaxBackground::Layer& layer,
    sf::Texture& tex, float factor, bool tileX) {
    if (tileX) tex.setRepeated(true);
    layer.sprite = std::make_unique<sf::Sprite>(tex);
    layer.parallaxFactor = factor;
    layer.tileX = tileX;
}

// ---------- public methods ----------
void ParallaxBackground::setFarLayer(SceneResources& res, int textureId,
    float factor, bool tileX) {
    if (!res.textures.isLoaded(textureId))
        throw std::runtime_error("Far layer texture not loaded");
    setLayerTexture(farLayer, res.textures.get(textureId), factor, tileX);
}

bool ParallaxBackground::addMidLayer(SceneResources& res, int textureId,
    float factor, bool tileX) {
    if (midLayers.size() >= MAX_MID) return false;
    if (!res.textures.isLoaded(textureId)) return false;
    Layer l;
    setLayerTexture(l, res.textures.get(textureId), factor, tileX);
    midLayers.push_back(std::move(l));
    return true;
}

void ParallaxBackground::setGroundLayer(SceneResources& res, int textureId,
    const std::vector<sf::Vector2f>& walkablePoly,
    bool tileX) {
    if (!res.textures.isLoaded(textureId))
        throw std::runtime_error("Ground layer texture not loaded");
    setLayerTexture(groundLayer, res.textures.get(textureId), 1.0f, tileX);
    walkablePolygon = walkablePoly;
    setSpawnX(0.0f);
}

bool ParallaxBackground::addForegroundLayer(SceneResources& res, int textureId,
    float factor, bool tileX) {
    if (foregroundLayers.size() >= MAX_FG) return false;
    if (!res.textures.isLoaded(textureId)) return false;
    Layer l;
    setLayerTexture(l, res.textures.get(textureId), factor, tileX);
    foregroundLayers.push_back(std::move(l));
    return true;
}

void ParallaxBackground::drawLayer(sf::RenderWindow& window, Layer& layer,
    const sf::Vector2f& cameraOffset) {
    if (!layer.sprite) return;

    float factor = layer.parallaxFactor;
    sf::Vector2f basePos = { -cameraOffset.x * factor, -cameraOffset.y * factor };

    if (layer.tileX) {
        sf::Vector2u texSize = layer.sprite->getTexture().getSize();
        layer.sprite->setTextureRect(
            sf::IntRect({ 0, 0 }, { int(texSize.x * TILE_COUNT), int(texSize.y) }));
        basePos.x = -fmod(cameraOffset.x * factor, (float)texSize.x);
        basePos.y = -cameraOffset.y * factor;
    }

    layer.sprite->setPosition({ basePos });
    window.draw(*layer.sprite);
}


void ParallaxBackground::draw(sf::RenderWindow& window, const sf::Vector2f& cameraOffset) {

    auto drawOne = [&](Layer& l, const sf::Vector2f& extra = { 0.f, 0.f }) {
        if (!l.sprite) return;
        float f = l.parallaxFactor;
        sf::Vector2f pos = extra - cameraOffset * f;
        if (l.tileX) {
            sf::Vector2u ts = l.sprite->getTexture().getSize();
            l.sprite->setTextureRect(sf::IntRect({ 0,0 }, { int(ts.x * TILE_COUNT), int(ts.y) }));
            pos.x = -fmod(cameraOffset.x * f, (float)ts.x);
            pos.y = extra.y - cameraOffset.y * f;
        }
        l.sprite->setPosition({ pos });
        window.draw(*l.sprite);
        };


    // Far
    drawOne(farLayer);
    // Mid
    for (auto& l : midLayers) drawOne(l);
    // Ground – offset so its bottom edge sits at floorY
    //float groundTexH = (float)groundLayer.sprite->getTexture().getSize().y;
   // drawOne(groundLayer);//, { 0.f, floorY - groundTexH });
    drawOne(groundLayer);
}



void ParallaxBackground::drawForeground(sf::RenderWindow& window, const sf::Vector2f& cameraOffset) {

    auto drawOne = [&](Layer& l, const sf::Vector2f& extra = { 0.f, 0.f }) {
        if (!l.sprite) return;
        float f = l.parallaxFactor;
        sf::Vector2f pos = extra - cameraOffset * f;
        if (l.tileX) {
            sf::Vector2u ts = l.sprite->getTexture().getSize();
            l.sprite->setTextureRect(sf::IntRect({ 0,0 }, { int(ts.x * TILE_COUNT), int(ts.y) }));
            pos.x = -fmod(cameraOffset.x * f, (float)ts.x);
            pos.y = extra.y - cameraOffset.y * f;
        }
        l.sprite->setPosition({ pos });
        window.draw(*l.sprite);
        };
    // Foreground
    for (auto& l : foregroundLayers) drawOne(l);

}