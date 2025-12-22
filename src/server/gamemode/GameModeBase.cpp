#include "server/gamemode/GameModeBase.hpp"

#include "al/Library/Play/Layout/SimpleLayoutAppearWaitEnd.h"

#include "game/Layout/CoinCounter.h"
#include "game/Layout/MapMini.h"

void GameModeBase::init(const GameModeInitInfo& info) {
    mSceneObjHolder = info.mSceneObjHolder;
    mMode = info.mMode;
    mCurScene = (StageScene*)info.mScene;
    mPuppetHolder = info.mPuppetHolder;
}

void GameModeBase::begin() {
    mIsActive = true;

    if (!isUseNormalUI()) {
        CoinCounter* coinCollect = mCurScene->stageSceneLayout->mCoinCollectLyt;
        CoinCounter* coinCounter = mCurScene->stageSceneLayout->mCoinCountLyt;
        MapMini* compass = mCurScene->stageSceneLayout->mMapMiniLyt;
        al::SimpleLayoutAppearWaitEnd* playGuideLyt = mCurScene->stageSceneLayout->mPlayGuideMenuLyt;

        if (coinCounter->mIsAlive)
            coinCounter->tryEnd();
        if (coinCollect->mIsAlive)
            coinCollect->tryEnd();
        if (compass->mIsAlive)
            compass->end();
        if (playGuideLyt->mIsAlive)
            playGuideLyt->end();
    }
}

void GameModeBase::end() {
    mIsActive = false;

    if (!isUseNormalUI()) {
        CoinCounter* coinCollect = mCurScene->stageSceneLayout->mCoinCollectLyt;
        CoinCounter* coinCounter = mCurScene->stageSceneLayout->mCoinCountLyt;
        MapMini* compass = mCurScene->stageSceneLayout->mMapMiniLyt;
        al::SimpleLayoutAppearWaitEnd* playGuideLyt = mCurScene->stageSceneLayout->mPlayGuideMenuLyt;

        if (!coinCounter->mIsAlive)
            coinCounter->tryStart();
        if (!coinCollect->mIsAlive)
            coinCollect->tryStart();
        if (!compass->mIsAlive)
            compass->appearSlideIn();
        if (!playGuideLyt->mIsAlive)
            playGuideLyt->appear();
    }
}