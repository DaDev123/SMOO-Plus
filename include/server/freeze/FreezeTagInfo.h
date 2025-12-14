#pragma once

#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/freeze/FreezeTagScore.hpp"

enum FreezeState { // Runner team player's state
    ALIVE = 0,
    FREEZE = 1
};

struct FreezeTagInfo : GameModeInfoBase {
    FreezeTagInfo() { mMode = GameMode::FREEZETAG; }
    bool mIsPlayerRunner = true;
    float mFreezeIconSize = 0.f;
    FreezeState mIsPlayerFreeze = FreezeState::ALIVE;

    bool mIsRound = false;
    int mFreezeCount = 0;
    FreezeTagScore mPlayerTagScore;
    GameTime mRoundTimer;

    sead::PtrArray<PuppetInfo> mRunnerPlayers;
    sead::PtrArray<PuppetInfo> mChaserPlayers;

    int mRoundLength = 10; // Length of rounds in minutes
    bool mIsHostMode = false;

    bool mIsDebugMode = false;
};