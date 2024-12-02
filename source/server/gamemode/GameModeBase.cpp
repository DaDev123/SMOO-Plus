#include "server/gamemode/GameModeBase.hpp"

#include "server/Client.hpp"

void GameModeBase::init(const GameModeInitInfo& info) {
    mSceneObjHolder = info.mSceneObjHolder;
    mMode           = info.mMode;
    mCurScene       = (StageScene*)info.mScene;
    mPuppetHolder   = info.mPuppetHolder;
}

void GameModeBase::begin() {
    mIsActive = true;

    if (!isUseNormalUI()) {
        CoinCounter*                   coinCollect  = mCurScene->mSceneLayout->mCoinCollectLyt;
        CoinCounter*                   coinCounter  = mCurScene->mSceneLayout->mCoinCountLyt;
        MapMini*                       compass      = mCurScene->mSceneLayout->mMapMiniLyt;
        al::SimpleLayoutAppearWaitEnd* playGuideLyt = mCurScene->mSceneLayout->mPlayGuideMenuLyt;

        if (coinCounter->mIsAlive)  { coinCounter->tryEnd(); }
        if (coinCollect->mIsAlive)  { coinCollect->tryEnd(); }
        if (compass->mIsAlive)      { compass->end();        }
        if (playGuideLyt->mIsAlive) { playGuideLyt->end();   }
    }

    Client::sendGameModeInfPacket();
}

void GameModeBase::end() {
    mIsActive = false;

    if (!isUseNormalUI()) {
        CoinCounter*                   coinCollect  = mCurScene->mSceneLayout->mCoinCollectLyt;
        CoinCounter*                   coinCounter  = mCurScene->mSceneLayout->mCoinCountLyt;
        MapMini*                       compass      = mCurScene->mSceneLayout->mMapMiniLyt;
        al::SimpleLayoutAppearWaitEnd* playGuideLyt = mCurScene->mSceneLayout->mPlayGuideMenuLyt;

        if (!coinCounter->mIsAlive)  { coinCounter->tryStart();  }
        if (!coinCollect->mIsAlive)  { coinCollect->tryStart();  }
        if (!compass->mIsAlive)      { compass->appearSlideIn(); }
        if (!playGuideLyt->mIsAlive) { playGuideLyt->appear();   }
    }

    Client::sendGameModeInfPacket();
}
