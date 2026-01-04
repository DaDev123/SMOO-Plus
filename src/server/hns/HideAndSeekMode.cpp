#include "server/hns/HideAndSeekMode.hpp"

#include "al/Library/Camera/CameraUtil.h"
#include "al/Library/Controller/InputFunction.h"
#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"

#include "game/Layout/CoinCounter.h"
#include "game/Layout/MapMini.h"
#include "game/Player/HackCap.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Player/PlayerAnimator.h"
#include "game/Player/PlayerFunction.h"
#include "game/System/GameDataFunction.h"
#include "game/Util/ActorDimensionKeeper.h"
#include "game/Util/ObjUtil.h"

#include <heap/seadHeap.h>

#include "helpers.hpp"
#include "imgui.h"
#include "layouts/HideAndSeekIcon.h"
#include "logger.hpp"
#include "rs/util.hpp"
#include "server/Client.hpp"
#include "server/DeltaTime.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/gamemode/GameModeTimer.hpp"

HideAndSeekMode::HideAndSeekMode(const char* name) : GameModeBase(name) {}

void HideAndSeekMode::init(const GameModeInitInfo& info) {
    mSceneObjHolder = info.mSceneObjHolder;
    mMode = info.mMode;
    mCurScene = (StageScene*)info.mScene;
    mPuppetHolder = info.mPuppetHolder;

    GameModeInfoBase* curGameInfo = GameModeManager::instance()->getInfo<HideAndSeekInfo>();

    if (curGameInfo)
        Logger::log("Gamemode info found: %s %s\n", GameModeFactory::getModeString(curGameInfo->mMode), GameModeFactory::getModeString(info.mMode));
    else
        Logger::log("No gamemode info found\n");
    if (curGameInfo && curGameInfo->mMode == mMode) {
        mInfo = (HideAndSeekInfo*)curGameInfo;
        mModeTimer = new GameModeTimer(mInfo->mHidingTime);
        Logger::log("Reinitialized timer with time %d:%.2d\n", mInfo->mHidingTime.mMinutes, mInfo->mHidingTime.mSeconds);
    } else {
        if (curGameInfo)
            delete curGameInfo;  // attempt to destory previous info before creating new one

        mInfo = GameModeManager::instance()->createModeInfo<HideAndSeekInfo>();

        mModeTimer = new GameModeTimer();
    }

    mModeLayout = new HideAndSeekIcon("HideAndSeekIcon", *info.mLayoutInitInfo);
    mModeLayout->setCurScene(mCurScene);

    mModeLayout->showSeeking();

    mModeTimer->disableTimer();
}

void HideAndSeekMode::begin() {
    mModeLayout->appear();

    mIsFirstFrame = true;

    mSpectateIndex = -1;
    mPrevSpectateIndex = -2;

    mIsSpectating = false;  // Initialize spectate state

    if (!mInfo->mIsPlayerIt && !mIsSpectating) {
        mModeTimer->enableTimer();
        mModeLayout->showHiding();
    } else {
        mModeTimer->disableTimer();
        mModeLayout->showSeeking();
    }

    CoinCounter* coinCollect = mCurScene->stageSceneLayout->mCoinCollectLyt;
    CoinCounter* coinCounter = mCurScene->stageSceneLayout->mCoinCountLyt;
    MapMini* compass = mCurScene->stageSceneLayout->mMapMiniLyt;
    al::SimpleLayoutAppearWaitEnd* playGuideLyt = mCurScene->stageSceneLayout->mPlayGuideMenuLyt;

    mInvulnTime = 0;

    if (coinCounter->mIsAlive)
        coinCounter->tryEnd();
    if (coinCollect->mIsAlive)
        coinCollect->tryEnd();
    if (compass->mIsAlive)
        compass->end();
    if (playGuideLyt->mIsAlive)
        playGuideLyt->end();

    GameModeBase::begin();
}

void HideAndSeekMode::end() {
    mModeLayout->tryEnd();
    mCurScene->stageSceneLayout->start();

    mModeTimer->disableTimer();

    CoinCounter* coinCollect = mCurScene->stageSceneLayout->mCoinCollectLyt;
    CoinCounter* coinCounter = mCurScene->stageSceneLayout->mCoinCountLyt;
    MapMini* compass = mCurScene->stageSceneLayout->mMapMiniLyt;
    al::SimpleLayoutAppearWaitEnd* playGuideLyt = mCurScene->stageSceneLayout->mPlayGuideMenuLyt;

    mInvulnTime = 0.0f;

    if (!coinCounter->mIsAlive)
        coinCounter->tryStart();
    if (!coinCollect->mIsAlive)
        coinCollect->tryStart();
    if (!compass->mIsAlive)
        compass->appearSlideIn();
    if (!playGuideLyt->mIsAlive)
        playGuideLyt->appear();

    GameModeBase::end();
}

void HideAndSeekMode::update() {
    PlayerActorBase* playerBase = (PlayerActorBase*)rs::getPlayerActor(mCurScene);

    bool isYukimaru = !playerBase->getPlayerInfo();  // if PlayerInfo is a nullptr, that means we're
                                                     // dealing with the bound bowl racer

    if (mIsFirstFrame) {
        if (mInfo->mIsUseGravityCam && mTicket) {
            al::startCamera(mCurScene, mTicket, -1);
        }

        mIsFirstFrame = false;
    }

    // Handle spectate camera toggle with R + D-Pad Up
    if (al::isPadHoldR(-1) && al::isPadTriggerUp(-1)) {
        mIsSpectating = !mIsSpectating;

        if (mIsSpectating) {
            // Start spectating
            if (mTicket && !mTicket->mIsActiveCamera) {
                al::startCamera(mCurScene, mTicket, -1);
                al::requestStopCameraVerticalAbsorb(mCurScene);
            }

            // Disable timer if we're a hider and start spectating
            if (!mInfo->mIsPlayerIt) {
                mModeTimer->disableTimer();
            }

            Logger::log("Spectate mode enabled\n");
        } else {
            // Stop spectating
            if (mTicket && mTicket->mIsActiveCamera) {
                al::endCamera(mCurScene, mTicket, 0, false);
                al::requestStopCameraVerticalAbsorb(mCurScene);
            }

            // Re-enable timer if we're a hider and stop spectating
            if (!mInfo->mIsPlayerIt) {
                mModeTimer->enableTimer();
            }

            Logger::log("Spectate mode disabled\n");
        }
    }

    // Update spectate camera if active
    if (mIsSpectating && mTicket && mTicket->mIsActiveCamera) {
        updateSpectateCam(playerBase);
    }

    if (!mInfo->mIsPlayerIt) {
        if (mInvulnTime >= 5) {
            if (playerBase) {
                for (size_t i = 0; i < mPuppetHolder->getSize(); i++) {
                    PuppetInfo* curInfo = Client::getPuppetInfo(i);

                    if (!curInfo) {
                        Logger::log("Checking %d, hit bounds %d-%d\n", i, mPuppetHolder->getSize(), Client::getMaxPlayerCount());
                        break;
                    }

                    if (curInfo->isConnected && curInfo->isInSameStage && curInfo->isIt) {
                        float pupDist = al::calcDistance(playerBase,
                                                         curInfo->playerPos);  // TODO: remove distance calculations and use hit
                                                                               // sensors to determine this

                        if (!isYukimaru) {
                            if (pupDist < 200.f && ((PlayerActorHakoniwa*)playerBase)->mDimensionKeeper->mIs2D == curInfo->is2D) {
                                if (!PlayerFunction::isPlayerDeadStatus(playerBase)) {
                                    GameDataFunction::killPlayer(GameDataHolderWriter(this));
                                    playerBase->startDemoPuppetable();
                                    al::setVelocityZero(playerBase);
                                    rs::faceToCamera(playerBase);
                                    ((PlayerActorHakoniwa*)playerBase)->mAnimator->endSubAnim();
                                    ((PlayerActorHakoniwa*)playerBase)->mAnimator->startAnimDead();

                                    mInfo->mIsPlayerIt = true;
                                    mModeTimer->disableTimer();
                                    mModeLayout->showSeeking();

                                    // Exit spectate mode when caught
                                    mIsSpectating = false;
                                    if (mTicket && mTicket->mIsActiveCamera) {
                                        al::endCamera(mCurScene, mTicket, 0, false);
                                        al::requestStopCameraVerticalAbsorb(mCurScene);
                                    }

                                    Client::sendTagInfPacket();
                                }
                            } else if (PlayerFunction::isPlayerDeadStatus(playerBase)) {
                                mInfo->mIsPlayerIt = true;
                                mModeTimer->disableTimer();
                                mModeLayout->showSeeking();

                                // Exit spectate mode when caught
                                mIsSpectating = false;
                                if (mTicket && mTicket->mIsActiveCamera) {
                                    al::endCamera(mCurScene, mTicket, 0, false);
                                    al::requestStopCameraVerticalAbsorb(mCurScene);
                                }

                                Client::sendTagInfPacket();
                            }
                        }
                    }
                }
            }

        } else {
            mInvulnTime += Time::deltaTime;
        }

        // Only update timer if not spectating
        if (!mIsSpectating) {
            mModeTimer->updateTimer();
        }

    } else {
        if (!mIsSpectating) {
            mModeTimer->timerControl();
        }
    }

    if (mInfo->mIsUseGravity && !isYukimaru) {
        sead::Vector3f gravity;
        if (rs::calcOnGroundNormalOrGravityDir(&gravity, playerBase, playerBase->getPlayerCollision())) {
            gravity = -gravity;
            al::normalize(&gravity);
            al::setGravity(playerBase, gravity);
            al::setGravity(((PlayerActorHakoniwa*)playerBase)->mHackCap, gravity);
        }

        if (al::isPadHoldL(-1)) {
            if (al::isPadTriggerRight(-1)) {
                if (al::isActiveCamera(mTicket)) {
                    al::endCamera(mCurScene, mTicket, -1, false);
                    mInfo->mIsUseGravityCam = false;
                } else {
                    al::startCamera(mCurScene, mTicket, -1);
                    mInfo->mIsUseGravityCam = true;
                }
            }
        } else if (al::isPadTriggerZL(-1)) {
            if (al::isPadTriggerLeft(-1)) {
                killMainPlayer(((PlayerActorHakoniwa*)playerBase));
            }
        }
    }

    if (al::isPadTriggerUp(-1) && !al::isPadHoldR(-1)) {
        mInfo->mIsPlayerIt = !mInfo->mIsPlayerIt;

        mModeTimer->toggleTimer();

        if (!mInfo->mIsPlayerIt) {
            mInvulnTime = 0;
            mModeLayout->showHiding();
        } else {
            mModeLayout->showSeeking();
        }

        Client::sendTagInfPacket();
    }

    mInfo->mHidingTime = mModeTimer->getTime();
}

void HideAndSeekMode::debugMenuControls() {
    ImGui::Text("- L + ← | Enable/disable Hide & Seek [H&S]\n");
    ImGui::Text("- [H&S] ↑ | Switch between hider and seeker\n");
    ImGui::Text("- [H&S] R + ↑ | Toggle Spectator Mode [Spectator]\n");
    ImGui::Text("- [H&S][Hider] ← | Decrease hiding time\n");
    ImGui::Text("- [H&S][Hider] → | Increase hiding time\n");
    ImGui::Text("- [H&S][Hider] L + ↓ | Reset hiding time\n");
    ImGui::Text("- [H&S][Spectator] ← / → | Switch target\n");
    ImGui::Text("- [H&S][Gravity] L + → | Toggle gravity camera\n");
}