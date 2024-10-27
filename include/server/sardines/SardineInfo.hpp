#pragma once

#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"

struct SardineInfo : GameModeInfoBase {
    SardineInfo() {
        mMode = GameMode::SARDINE;
    }
    bool mIsIt            = false;
    bool mIsUseGravityCam = false;

    static bool mIsUseGravity;
    static bool mIsTether;
    static bool mIsTetherSnap;

    static bool mHasMarioCollision;
    static bool mHasMarioBounce;
    static bool mHasCappyCollision;
    static bool mHasCappyBounce;

    GameTime mHidingTime;

    inline bool isPlayerAlone() const { return !mIsIt; }
    inline bool isPlayerPack()  const { return  mIsIt; }
};
