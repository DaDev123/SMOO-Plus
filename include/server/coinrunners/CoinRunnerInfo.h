#pragma once

#include "puppets/PuppetInfo.h"
#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/coinrunners/CoinRunnerScore.hpp"

enum CoinState { // Runner team player's state
    ALIVECoin  = 0,
    Coin = 1,
};

struct CoinRunnerInfo : GameModeInfoBase {
    CoinRunnerInfo() {
        mMode = GameMode::COINRUNNER;
    }

    bool        mIsPlayerRunner = true;
    float       mCoinIconSize = 0.f;
    CoinState mIsPlayerCoin = CoinState::ALIVECoin;

    bool     mIsRound     = false;
    int      mCoinCount = 0; // how often runners were frozen in the current round (including refreezes after unfreeze)
    GameTime mRoundTimer;

    sead::PtrArray<PuppetInfo> mRunnerPlayers;
    sead::PtrArray<PuppetInfo> mChaserPlayers;
    sead::PtrArray<PuppetInfo> mOtherPlayers;

    static CoinRunnerScore mPlayerTagScore;
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
    inline bool     isPlayerFrozen()   const { return  mIsPlayerCoin;                          }
    inline bool     isPlayerUnfrozen() const { return !mIsPlayerCoin;                          }
    inline int      runners()          const { return  mRunnerPlayers.size() + isPlayerRunner(); }
    inline int      chasers()          const { return  mChaserPlayers.size() + isPlayerChaser(); }
    inline int      others()           const { return  mOtherPlayers.size();                     }
    inline uint16_t getScore()         const { return  mPlayerTagScore.mScore;                   }
};
