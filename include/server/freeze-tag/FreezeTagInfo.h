#pragma once

#include "puppets/PuppetInfo.h"
#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/freeze-tag/FreezeTagScore.hpp"

enum FreezeState { // Runner team player's state
    ALIVE  = 0,
    FREEZE = 1,
};

struct FreezeTagInfo : GameModeInfoBase {
    FreezeTagInfo() {
        mMode = GameMode::FREEZETAG;
    }

    bool        mIsPlayerRunner = true;
    float       mFreezeIconSize = 0.f;
    FreezeState mIsPlayerFreeze = FreezeState::ALIVE;

    bool     mIsRound     = false;
    int      mFreezeCount = 0; // how often runners were frozen in the current round (including refreezes after unfreeze)
    GameTime mRoundTimer;

    sead::PtrArray<PuppetInfo> mRunnerPlayers;
    sead::PtrArray<PuppetInfo> mChaserPlayers;
    sead::PtrArray<PuppetInfo> mOtherPlayers;

    static FreezeTagScore mPlayerTagScore;
    static int            mRoundLength; // Length of rounds in minutes
    static bool           mIsHostMode;  // can start/cancel round
    static bool           mIsDebugMode; // trigger manual actions that are normally automatic (might break game logic)

    static bool mHasMarioCollision;
    static bool mHasMarioBounce;
    static bool mHasCappyCollision;
    static bool mHasCappyBounce;

    inline bool     isHost()           const { return  mIsHostMode;                              }
    inline bool     isRound()          const { return  mIsRound;                                 }
    inline bool     isPlayerRunner()   const { return  mIsPlayerRunner;                          }
    inline bool     isPlayerChaser()   const { return !mIsPlayerRunner;                          }
    inline bool     isPlayerFrozen()   const { return  mIsPlayerFreeze;                          }
    inline bool     isPlayerUnfrozen() const { return !mIsPlayerFreeze;                          }
    inline int      runners()          const { return  mRunnerPlayers.size() + isPlayerRunner(); }
    inline int      chasers()          const { return  mChaserPlayers.size() + isPlayerChaser(); }
    inline int      others()           const { return  mOtherPlayers.size();                     }
    inline uint16_t getScore()         const { return  mPlayerTagScore.mScore;                   }
};
