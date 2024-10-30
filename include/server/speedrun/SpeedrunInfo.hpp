#pragma once

#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"

struct SpeedrunInfo : GameModeInfoBase {
    SpeedrunInfo() {
        mMode = GameMode::SPEEDRUN;
    }
    bool     mIsPlayerIt          = false;
    bool     mIsUseGravityCam     = false;
    bool     mIsUseSlipperyGround = true;
    GameTime mHidingTime;

    static bool mIsUseGravity;

    static bool mHasMarioCollision;
    static bool mHasMarioBounce;
    static bool mHasCappyCollision;
    static bool mHasCappyBounce;

    inline bool isPlayerSeeking() const { return  mIsPlayerIt; }
    inline bool isPlayerHiding()  const { return !mIsPlayerIt; }
};
