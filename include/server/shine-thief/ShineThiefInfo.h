#pragma once

#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/shine-thief/ShineThiefScore.hpp"

enum class ShineThiefTeam : uint8_t { NONE = 0, TEAM_1 = 1, TEAM_2 = 2 };

struct ShineThiefInfo : GameModeInfoBase {
    ShineThiefInfo() { mMode = GameMode::SHINETHIEF; }

    // Player state
    bool mIsPlayerHolder = false;
    float mShineThiefIconSize = 0.f;

    // Round info
    bool mIsRound = false;
    ShineThiefScore mPlayerTagScore;
    GameTime mRoundTimer;
    sead::Vector3f shinePos;

    // Player lists - holders vs thieves
    sead::PtrArray<PuppetInfo> mHolderPlayers;
    sead::PtrArray<PuppetInfo> mThiefPlayers;

    // Team mode
    bool mIsTeamMode = false;
    ShineThiefTeam mPlayerTeam = ShineThiefTeam::NONE;
    uint16_t mTeam1Score = 0;
    uint16_t mTeam2Score = 0;
    sead::PtrArray<PuppetInfo> mTeam1Players;
    sead::PtrArray<PuppetInfo> mTeam2Players;

    // Configuration
    int mRoundLength = 10;
    bool mIsHostMode = true;
    bool mIsDebugMode = false;
};