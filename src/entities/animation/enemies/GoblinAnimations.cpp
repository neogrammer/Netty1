#include "GoblinAnimations.h"
#include <entities/animation/AnimationSet.h>
#include <network/NetTypes.h>

void initGoblinAnimations(AnimationSet& animSet, sf::Texture& sheet) {
    const int FRAME_W = 200, FRAME_H = 225;

    auto buildRow = [&](int row, AnimType type, int frameCount) {
        std::vector<sf::IntRect> right, left;
        for (int f = 0; f < frameCount; ++f) {
            right.push_back({ {f * FRAME_W, row * FRAME_H}, {FRAME_W, FRAME_H} });
            left.push_back({ {(f + 1) * FRAME_W, row * FRAME_H}, {-FRAME_W, FRAME_H} });
        }
        animSet.animRects[type][1] = right;
        animSet.animRects[type][0] = left;
        };

    // Row 0: Idle (8 frames, loop)
    buildRow(0, AnimType::Idle, 8);
    animSet.animMap[AnimType::Idle] = &sheet;
    animSet.frameCounts[AnimType::Idle] = 8;
    animSet.animDurations[AnimType::Idle] = 0.12f;
    animSet.loops[AnimType::Idle] = true;

    // Row 2: Run → Walk (8 frames, loop)
    buildRow(2, AnimType::Walk, 8);
    animSet.animMap[AnimType::Walk] = &sheet;
    animSet.frameCounts[AnimType::Walk] = 8;
    animSet.animDurations[AnimType::Walk] = 0.08f;
    animSet.loops[AnimType::Walk] = true;

    // Row 3: Attack → Attack1 (8 frames)
    buildRow(3, AnimType::Attack1, 8);
    animSet.animMap[AnimType::Attack1] = &sheet;
    animSet.frameCounts[AnimType::Attack1] = 8;
    animSet.animDurations[AnimType::Attack1] = 0.10f;
    animSet.loops[AnimType::Attack1] = false;

    // Attack2/Attack3 reuse Attack1
    animSet.animMap[AnimType::Attack2] = &sheet;
    animSet.animRects[AnimType::Attack2] = animSet.animRects[AnimType::Attack1];
    animSet.frameCounts[AnimType::Attack2] = 8;
    animSet.animDurations[AnimType::Attack2] = 0.10f;
    animSet.loops[AnimType::Attack2] = false;

    animSet.animMap[AnimType::Attack3] = &sheet;
    animSet.animRects[AnimType::Attack3] = animSet.animRects[AnimType::Attack1];
    animSet.frameCounts[AnimType::Attack3] = 8;
    animSet.animDurations[AnimType::Attack3] = 0.10f;
    animSet.loops[AnimType::Attack3] = false;

    // Row 4: Hurt → Hit (8 frames)
    buildRow(4, AnimType::Hit, 8);
    animSet.animMap[AnimType::Hit] = &sheet;
    animSet.frameCounts[AnimType::Hit] = 8;
    animSet.animDurations[AnimType::Hit] = 0.08f;
    animSet.loops[AnimType::Hit] = false;

    // Row 5: Die → Death (8 frames)
    buildRow(5, AnimType::Death, 8);
    animSet.animMap[AnimType::Death] = &sheet;
    animSet.frameCounts[AnimType::Death] = 8;
    animSet.animDurations[AnimType::Death] = 0.15f;
    animSet.loops[AnimType::Death] = false;

    // Row 6: JumpUp (1 frame)
    buildRow(6, AnimType::JumpUp, 1);
    animSet.animMap[AnimType::JumpUp] = &sheet;
    animSet.frameCounts[AnimType::JumpUp] = 1;
    animSet.animDurations[AnimType::JumpUp] = 0.10f;
    animSet.loops[AnimType::JumpUp] = false;

    // Row 7: FallDown (1 frame)
    buildRow(7, AnimType::JumpDown, 1);
    animSet.animMap[AnimType::JumpDown] = &sheet;
    animSet.frameCounts[AnimType::JumpDown] = 1;
    animSet.animDurations[AnimType::JumpDown] = 0.10f;
    animSet.loops[AnimType::JumpDown] = false;
}