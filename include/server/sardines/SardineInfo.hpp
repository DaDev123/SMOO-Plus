#pragma once

#include "server/gamemode/GameMode.hpp"
#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"

struct SardineInfo : GameModeInfoBase {
    SardineInfo() {
        mMode = GameMode::SARDINE;
    }
    bool mIsIt            = false;
    bool mIsUseGravity    = false;
    bool mIsUseGravityCam = false;

    bool mIsTether     = false;
    bool mIsTetherSnap = false;

    GameTime mHidingTime;
};
