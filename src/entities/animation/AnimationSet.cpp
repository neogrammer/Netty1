#include "AnimationSet.h"
#include <res/Cfg.h>
#include <res/SceneResources.h>

void initPlayerAnimations(AnimationSet& animSet)
{
    auto* tex = &Cfg::textures.get((int)Cfg::Textures::Hero1_Sheet);

    const int FRAME_W = 252;
    const int FRAME_H = 252;

    // Collision box for gameplay — same across all frames
    // Offset from sprite origin: (48, 42), size: (37, 40)
    // We'll store this elsewhere; animation just handles visuals.

    // Helper: fill one animation row.
    // All frames face right on the sheet; left-facing is handled by
    // setting the sprite scale to -1 (flip X) at render time.
    auto fillAnim = [&](AnimType type, int frameCount, int yRow,
        float duration, bool loop,
        std::optional<AnimType> next = std::nullopt)
        {
            animSet.animMap[type] = tex;
            animSet.frameCounts[type] = frameCount;
            animSet.animDurations[type] = duration;
            animSet.loops[type] = loop;
            animSet.nextAnim[type] = next;
            animSet.animRects[type] = std::array<std::vector<sf::IntRect>, 2>{};

            for (int i = 0; i < frameCount; ++i) {
                // Right-facing: normal rect
                sf::IntRect rectRight({ i * FRAME_W, yRow }, { FRAME_W, FRAME_H });
                animSet.animRects[type][1].push_back(rectRight);

                // Left-facing: flipped rect — start at right edge, negative width
                sf::IntRect rectLeft({ (i + 1) * FRAME_W, yRow }, { -FRAME_W, FRAME_H });
                animSet.animRects[type][0].push_back(rectLeft);
            }
        };

    // Row order from top to bottom:
    // Attack1 (7), Attack2 (6), Attack3 (9), Death (11),
    // GoingDown (3), GoingUp (3), Idle (10), Run (8), TakeHit (3)

    fillAnim(AnimType::Attack1, 7, FRAME_H * 0, 0.14f, false); // /*0, 0.07f*/
    fillAnim(AnimType::Attack2, 6, FRAME_H * 1, 0.14f, false); // /*0, 0.07f*/
    fillAnim(AnimType::Attack3, 9, FRAME_H * 2, 0.14f, false); // /*0, 0.07f*/
    fillAnim(AnimType::Death, 11, FRAME_H * 3, 0.12f, false);
    fillAnim(AnimType::JumpDown, 3, FRAME_H * 4, 0.10f, false);
    fillAnim(AnimType::JumpUp, 3, FRAME_H * 5, 0.10f, false);
    fillAnim(AnimType::Idle, 10, FRAME_H * 6, 0.10f, true);
    fillAnim(AnimType::Walk, 8, FRAME_H * 7, 0.08f, true);
    fillAnim(AnimType::Hit, 3, FRAME_H * 8, 0.08f, false);
}
