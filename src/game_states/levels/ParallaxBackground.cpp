//#include "ParallaxBackground.h"
//#include <cmath>
//
//// ------------------------------------------------------------
//void ParallaxBackground::setFarLayer(SceneResources& res, int textureId, float factor, bool tileX) {
//    if (!res.textures.isLoaded(textureId))
//        throw std::runtime_error("Far layer texture not loaded");
//    auto& tex = res.textures.get(textureId);
//    if (tileX) tex.setRepeated(true);
//    farLayer.sprite.setTexture(tex);
//    farLayer.parallaxFactor = factor;
//    farLayer.tileX = tileX;
//}
//
//bool ParallaxBackground::addMidLayer(SceneResources& res, int textureId, float factor, bool tileX) {
//    if (midLayers.size() >= MAX_MID) return false;
//    if (!res.textures.isLoaded(textureId)) return false;
//    auto& tex = res.textures.get(textureId);
//    if (tileX) tex.setRepeated(true);
//    Layer l;
//    l.sprite.setTexture(tex);
//    l.parallaxFactor = factor;
//    l.tileX = tileX;
//    midLayers.push_back(std::move(l));
//    return true;
//}
//
//void ParallaxBackground::setGroundLayer(SceneResources& res, int textureId,
//    const std::vector<sf::Vector2f>& walkablePoly,
//    bool tileX) {
//    if (!res.textures.isLoaded(textureId))
//        throw std::runtime_error("Ground layer texture not loaded");
//    auto& tex = res.textures.get(textureId);
//    if (tileX) tex.setRepeated(true);
//    groundLayer.sprite.setTexture(tex);
//    groundLayer.parallaxFactor = 1.0f;   // ground always moves 1:1 with camera
//    groundLayer.tileX = tileX;
//    walkablePolygon = walkablePoly;
//}
//
//bool ParallaxBackground::addForegroundLayer(SceneResources& res, int textureId, float factor, bool tileX) {
//    if (foregroundLayers.size() >= MAX_FG) return false;
//    if (!res.textures.isLoaded(textureId)) return false;
//    auto& tex = res.textures.get(textureId);
//    if (tileX) tex.setRepeated(true);
//    Layer l;
//    l.sprite.setTexture(tex);
//    l.parallaxFactor = factor;
//    l.tileX = tileX;
//    foregroundLayers.push_back(std::move(l));
//    return true;
//}
//
//// ------------------------------------------------------------
//void ParallaxBackground::drawLayer(sf::RenderWindow& window, Layer& layer,
//    const sf::Vector2f& cameraOffset) {
//    float factor = layer.parallaxFactor;
//    sf::Vector2f basePos = { -cameraOffset.x * factor, -cameraOffset.y * factor };
//
//    if (layer.tileX) {
//        // Texture is already set to repeat – use a wide texture rect
//        sf::Vector2u texSize = layer.sprite.getTexture()->getSize();
//        layer.sprite.setTextureRect(
//            sf::IntRect(0, 0, texSize.x * TILE_COUNT, texSize.y));
//        // Position the sprite so the repeating pattern aligns smoothly
//        layer.sprite.setPosition({ -fmod(cameraOffset.x * factor, (float)texSize.x),
//                                   basePos.y });
//    }
//    else {
//        layer.sprite.setPosition(basePos);
//    }
//
//    window.draw(layer.sprite);
//}
//
//void ParallaxBackground::draw(sf::RenderWindow& window, const sf::Vector2f& cameraOffset) {
//    // 1. Far background
//    if (farLayer.sprite.getTexture())
//        drawLayer(window, farLayer, cameraOffset);
//
//    // 2. Mid layers
//    for (auto& layer : midLayers)
//        drawLayer(window, layer, cameraOffset);
//
//    // 3. Ground
//    if (groundLayer.sprite.getTexture())
//        drawLayer(window, groundLayer, cameraOffset);
//
//    // 4. Foreground layers
//    for (auto& layer : foregroundLayers)
//        drawLayer(window, layer, cameraOffset);
//}

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

    layer.sprite->setPosition(basePos);
    window.draw(*layer.sprite);
}


//void ParallaxBackground::drawLayer(sf::RenderWindow& window, Layer& layer,
//    const sf::Vector2f& cameraOffset) {
//    if (!layer.sprite) return;
//
//    float factor = layer.parallaxFactor;
//    // NEW: base position is groundOffset - cameraOffset * factor
//    sf::Vector2f basePos = groundOffset - cameraOffset * factor;
//
//    if (layer.tileX) {
//        sf::Vector2u texSize = layer.sprite->getTexture().getSize();
//        layer.sprite->setTextureRect(
//            sf::IntRect({ 0, 0 }, { int(texSize.x * TILE_COUNT), int(texSize.y) }));
//        basePos.x = -fmod((cameraOffset.x - groundOffset.x) * factor, (float)texSize.x);
//        basePos.y = groundOffset.y - cameraOffset.y * factor;
//    }
//
//    layer.sprite->setPosition(basePos);
//    window.draw(*layer.sprite);
//}


//void ParallaxBackground::drawLayer(sf::RenderWindow& window, Layer& layer,
//    const sf::Vector2f& cameraOffset) {
//    if (!layer.sprite) return;
//
//    float factor = layer.parallaxFactor;
//    // NEW: base position is relative to the anchor, not absolute camera offset
//    sf::Vector2f basePos = (anchorOffset - cameraOffset) * factor;
//
//    if (layer.tileX) {
//        sf::Vector2u texSize = layer.sprite->getTexture().getSize();
//        layer.sprite->setTextureRect(
//            sf::IntRect({ 0, 0 }, { int(texSize.x * TILE_COUNT), int(texSize.y) }));
//        // keep x‑tiling smooth
//        basePos.x = -fmod((cameraOffset.x - anchorOffset.x) * factor, (float)texSize.x);
//        basePos.y = (anchorOffset.y - cameraOffset.y) * factor;
//    }
//
//    layer.sprite->setPosition(basePos);
//    window.draw(*layer.sprite);
//}


// ---------- drawing ----------
//void ParallaxBackground::drawLayer(sf::RenderWindow& window, Layer& layer,
//    const sf::Vector2f& cameraOffset) {
//    if (!layer.sprite) return;
//
//    float factor = layer.parallaxFactor;
//    sf::Vector2f basePos = { -cameraOffset.x * factor, -cameraOffset.y * factor };
//
//    if (layer.tileX) {
//        sf::Vector2u texSize = layer.sprite->getTexture().getSize();
//        layer.sprite->setTextureRect(sf::IntRect({ 0, 0 }, { int(texSize.x * TILE_COUNT), int(texSize.y) }));
//        layer.sprite->setPosition({ -fmod(cameraOffset.x * factor, (float)texSize.x), basePos.y });
//    }
//    else {
//        layer.sprite->setPosition(basePos);
//    }
//
//    window.draw(*layer.sprite);
//}

void ParallaxBackground::draw(sf::RenderWindow& window, const sf::Vector2f& cameraOffset) {
   
    auto drawOne = [&](Layer& l, const sf::Vector2f& extra = { 0.f, 0.f }) {
        if (!l.sprite) return;
        float f = l.parallaxFactor;
        // Shift camera offset by spawnX so world x=0 aligns with the spawn point
        sf::Vector2f shiftedOffset = cameraOffset - sf::Vector2f(spawnX, 0.f);
        sf::Vector2f pos = extra - shiftedOffset * f;
        if (l.tileX) {
            sf::Vector2u ts = l.sprite->getTexture().getSize();
            l.sprite->setTextureRect(sf::IntRect({ 0,0 }, { int(ts.x * TILE_COUNT), int(ts.y) }));
            pos.x = -fmod(shiftedOffset.x * f, (float)ts.x);
            pos.y = extra.y - shiftedOffset.y * f;
        }
        l.sprite->setPosition(pos);
        window.draw(*l.sprite);
        };

    // Far
    drawOne(farLayer);
    // Mid
    for (auto& l : midLayers) drawOne(l);
    // Ground – offset so its bottom edge sits at floorY
    //float groundTexH = (float)groundLayer.sprite->getTexture().getSize().y;
    drawOne(groundLayer);//, { 0.f, floorY - groundTexH });
    // Foreground
    for (auto& l : foregroundLayers) drawOne(l);
}


//void ParallaxBackground::draw(sf::RenderWindow& window, const sf::Vector2f& cameraOffset) {
//    // 1. Far background
//    drawLayer(window, farLayer, cameraOffset);
//
//    // 2. Mid layers
//    for (auto& layer : midLayers)
//        drawLayer(window, layer, cameraOffset);
//
//    // 3. Ground – offset so the floor sits at floorY in world space
//    if (groundLayer.sprite) {
//        float textureHeight = (float)groundLayer.sprite->getTexture().getSize().y;
//        sf::Vector2f groundOffset(0.f, floorY - textureHeight);
//        // Temporarily shift the ground sprite, then draw it
//        sf::Vector2f savedPos = groundLayer.sprite->getPosition();
//        drawLayer(window, groundLayer, cameraOffset);
//        // Apply the offset after drawLayer's default positioning
//        groundLayer.sprite->setPosition(groundLayer.sprite->getPosition() + groundOffset);
//        // Wait – this is messy. Let's do it cleanly in drawLayer with an extra offset parameter.
//    }
//    // Better approach below...
//}


//void ParallaxBackground::draw(sf::RenderWindow& window, const sf::Vector2f& cameraOffset) {
//    // 1. Far background
//    drawLayer(window, farLayer, cameraOffset);
//
//    // 2. Mid layers
//    for (auto& layer : midLayers)
//        drawLayer(window, layer, cameraOffset);
//
//    // 3. Ground
//    drawLayer(window, groundLayer, cameraOffset);
//
//    // 4. Foreground layers
//    for (auto& layer : foregroundLayers)
//        drawLayer(window, layer, cameraOffset);
//}
