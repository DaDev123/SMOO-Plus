#pragma once

#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/hotpotato/HotPotatoScore.hpp"

enum HotState { // Runner team player's state
    HOTALIVE = 0,
    HOTFREEZE = 1
};

struct HotPotatoInfo : GameModeInfoBase {
    HotPotatoInfo() { mMode = GameMode::HOTPOTATO; }
    bool mIsPlayerRunner = true;
    float mFreezeIconSize = 0.f;
    HotState mIsPlayerFreeze = HotState::HOTALIVE;

    bool mIsRound = false;
    int mFreezeCount = 0;
    HotPotatoScore mPlayerTagScore;
    GameTime mRoundTimer;

    sead::PtrArray<PuppetInfo> mRunnerPlayers;
    sead::PtrArray<PuppetInfo> mChaserPlayers;

    int mRoundLength = 10; // Length of rounds in minutes
    bool mIsHostMode = false;

    bool mIsDebugMode = false;
};