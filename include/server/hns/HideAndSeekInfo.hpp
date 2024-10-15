#pragma once

#include "server/gamemode/GameMode.hpp"
#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"

struct HideAndSeekInfo : GameModeInfoBase {
    HideAndSeekInfo() {
        mMode = GameMode::HIDEANDSEEK;
    }
    bool     mIsPlayerIt          = false;
    bool     mIsUseGravity        = false;
    bool     mIsUseGravityCam     = false;
    bool     mIsUseSlipperyGround = true;
    GameTime mHidingTime;
};
