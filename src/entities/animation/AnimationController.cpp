#include "AnimationController.h"
#include <network/server/ServerGameLoop.h>  // for PlayerSlot
#include <network/server/AnimConfig.h>
#include <cstdio>

AnimType AnimationController::updateAnimation(PlayerSlot& slot, Entity& entity, uint32_t serverTick) {
    // Track idle time for grace period
    if (slot.dir == 0 && !slot.wantsAttack1 && !slot.wantsAttack2 &&
        !slot.wantsAttack3 && !slot.wantsJump) {
        slot.idleTicks++;
    }
    else {
        slot.idleTicks = 0;
    }

    AnimType current = static_cast<AnimType>(entity.animation);
    bool inAttack = (current == AnimType::Attack1 ||
        current == AnimType::Attack2 ||
        current == AnimType::Attack3);
    bool inJump = (current == AnimType::JumpUp || current == AnimType::JumpDown);

    uint32_t elapsed = serverTick - entity.animStartTick;
    bool animFinished = !animLoops(current) && elapsed >= animDurationTicks(current);

    uint32_t jumpElapsed = serverTick - slot.jumpStartTick;
    AnimType desired = current;

    // Jump transitions
    if (slot.isJumping && current == AnimType::JumpUp && jumpElapsed >= JUMP_UP_DURATION) {
        desired = AnimType::JumpDown;
    }
    else if (slot.isJumping && current == AnimType::JumpDown && jumpElapsed >= JUMP_TOTAL_DURATION) {
        desired = AnimType::Idle;
        slot.isJumping = false;
    }
    else if (slot.isJumping) {
        desired = current;
    }
    else if (current == AnimType::Death) {
        // no changes
    }
    else if (animFinished && inAttack) {
        if (slot.wantsAttack2 && current == AnimType::Attack1) {
            desired = AnimType::Attack2;
        }
        else if (slot.wantsAttack3 && current == AnimType::Attack2) {
            desired = AnimType::Attack3;
        }
        else if (slot.idleTicks >= IDLE_GRACE_TICKS) {
            desired = AnimType::Idle;
        }
        else {
            desired = AnimType::Walk;
        }
    }
    else if (!inAttack && !inJump) {
        if (slot.wantsAttack1) {
            desired = AnimType::Attack1;
            printf("[Server] Player wants Attack1\n");
        }
        else if (slot.wantsAttack2) {
            desired = AnimType::Attack2;
        }
        else if (slot.wantsAttack3) {
            desired = AnimType::Attack3;
        }
        else if (slot.wantsJump) {
            desired = AnimType::JumpUp;
            slot.isJumping = true;
            slot.jumpStartTick = serverTick;
            slot.jumpStartY = entity.y;
        }
        else if (slot.dir != 0) {
            desired = AnimType::Walk;
        }
        else if (slot.idleTicks >= IDLE_GRACE_TICKS) {
            desired = AnimType::Idle;
        }
        else {
            desired = AnimType::Walk;
        }
    }

    if (desired != current) {
        entity.animation = static_cast<uint8_t>(desired);
        entity.animStartTick = serverTick;
    }

    return desired;
}