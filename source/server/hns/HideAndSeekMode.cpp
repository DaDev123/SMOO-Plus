#include "server/hns/HideAndSeekMode.hpp"
#include <cmath>
#include "al/async/FunctorV0M.hpp"
#include "al/util.hpp"
#include "al/util/ControllerUtil.h"
#include "al/util/LiveActorUtil.h"
#include "game/GameData/GameDataHolderAccessor.h"
#include "game/Layouts/CoinCounter.h"
#include "game/Layouts/MapMini.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "heap/seadHeapMgr.h"
#include "logger.hpp"
#include "math/seadVector.h"
#include "packets/Packet.h"
#include "rs/util.hpp"
#include "rs/util/PlayerUtil.h"
#include "server/Client.hpp"
#include <heap/seadHeap.h>
#include <math.h>
#include "server/gamemode/GameModeManager.hpp"
#include "server/gamemode/GameModeFactory.hpp"

#include "basis/seadNew.h"
#include "server/hns/HideAndSeekConfigMenu.hpp"
#include "server/hns/HideAndSeekPacket.hpp"

HideAndSeekMode::HideAndSeekMode(const char* name) : GameModeBase(name) {}

void HideAndSeekMode::init(const GameModeInitInfo& info) {
    mSceneObjHolder = info.mSceneObjHolder;
    mMode           = info.mMode;
    mCurScene       = (StageScene*)info.mScene;
    mPuppetHolder   = info.mPuppetHolder;

    GameModeInfoBase* curGameInfo = GameModeManager::instance()->getInfo<GameModeInfoBase>();

    sead::ScopedCurrentHeapSetter heapSetter(GameModeManager::instance()->getHeap());

    if (curGameInfo) {
        Logger::log("Gamemode info found: %s %s\n", GameModeFactory::getModeString(curGameInfo->mMode), GameModeFactory::getModeString(info.mMode));
    } else {
        Logger::log("No gamemode info found\n");
    }

    if (curGameInfo && curGameInfo->mMode == mMode) {
        sead::ScopedCurrentHeapSetter heapSetter(GameModeManager::getSceneHeap());
        mInfo = (HideAndSeekInfo*)curGameInfo;
        mModeTimer = new GameModeTimer(mInfo->mHidingTime);
        Logger::log("Reinitialized timer with time %d:%.2d\n", mInfo->mHidingTime.mMinutes, mInfo->mHidingTime.mSeconds);
    } else {
        if (curGameInfo) {
            delete curGameInfo; // attempt to destory previous info before creating new one
        }

        mInfo = GameModeManager::instance()->createModeInfo<HideAndSeekInfo>();

        mModeTimer = new GameModeTimer();
    }

    sead::ScopedCurrentHeapSetter heapSetterr(GameModeManager::getSceneHeap());

    mModeLayout = new HideAndSeekIcon("HideAndSeekIcon", *info.mLayoutInitInfo);

    mModeLayout->showSeeking();

    mModeTimer->disableTimer();
}

void HideAndSeekMode::processPacket(Packet* _packet) {
    HideAndSeekPacket* packet     = (HideAndSeekPacket*)_packet;
    HnSUpdateType      updateType = packet->updateType();

    // if the packet is for our player, edit info for our player
    if (packet->mUserID == Client::getClientId()) {
        if (updateType & HnSUpdateType::TIME) {
            mInfo->mHidingTime.mMilliseconds = 0.0;
            mInfo->mHidingTime.mSeconds      = packet->seconds;
            mInfo->mHidingTime.mMinutes      = packet->minutes % 60;
            mInfo->mHidingTime.mHours        = packet->minutes / 60;
            mModeTimer->setTime(mInfo->mHidingTime);
        }

        if (updateType & HnSUpdateType::STATE) {
            updateTagState(packet->isIt);
        } else if (updateType & HnSUpdateType::TIME) {
            Client::sendGameModeInfPacket();
        }

        return;
    }

    PuppetInfo* other = Client::findPuppetInfo(packet->mUserID, false);
    if (!other) {
        return;
    }

    if (updateType & HnSUpdateType::STATE) {
        other->isIt = packet->isIt;
    }

    if (updateType & HnSUpdateType::TIME) {
        other->seconds = packet->seconds;
        other->minutes = packet->minutes;
    }
}

Packet* HideAndSeekMode::createPacket() {
    if (!isModeActive()) {
        DisabledGameModeInf* packet = new DisabledGameModeInf(Client::getClientId());
        return packet;
    }

    HideAndSeekPacket* packet = new HideAndSeekPacket();
    packet->mUserID    = Client::getClientId();
    packet->isIt       = isPlayerIt();
    packet->seconds    = mInfo->mHidingTime.mSeconds;
    packet->minutes    = mInfo->mHidingTime.mMinutes + mInfo->mHidingTime.mHours * 60;
    packet->setUpdateType(static_cast<HnSUpdateType>(HnSUpdateType::STATE | HnSUpdateType::TIME));
    return packet;
}

void HideAndSeekMode::begin() {
    unpause();

    mIsFirstFrame = true;
    mInvulnTime   = 0.0f;

    GameModeBase::begin();
}

void HideAndSeekMode::end() {
    pause();

    GameModeBase::end();
}

void HideAndSeekMode::pause() {
    GameModeBase::pause();

    mModeLayout->tryEnd();
    mModeTimer->disableTimer();
}

void HideAndSeekMode::unpause() {
    GameModeBase::unpause();

    mModeLayout->appear();

    if (mInfo->mIsPlayerIt) {
        mModeTimer->disableTimer();
        mModeLayout->showSeeking();
    } else {
        mModeTimer->enableTimer();
        mModeLayout->showHiding();
    }
}

void HideAndSeekMode::update() {
    PlayerActorBase* playerBase = rs::getPlayerActor(mCurScene);

    bool isYukimaru = !playerBase->getPlayerInfo(); // if PlayerInfo is a nullptr, that means we're dealing with the bound bowl racer

    if (mIsFirstFrame) {
        if (mInfo->mIsUseGravityCam && mTicket) {
            al::startCamera(mCurScene, mTicket, -1);
        }
        mIsFirstFrame = false;
    }

    if (rs::isActiveDemoPlayerPuppetable(playerBase)) {
        mInvulnTime = 0.0f; // if player is in a demo, reset invuln time
    }

    if (mInfo->mIsPlayerIt) {
        mModeTimer->timerControl();
    } else {
        if (mInvulnTime < 5) {
            mInvulnTime += Time::deltaTime;
        } else if (playerBase) {
            for (size_t i = 0; i < (size_t)mPuppetHolder->getSize(); i++) {
                PuppetInfo* other = Client::getPuppetInfo(i);
                if (!other) {
                    Logger::log("Checking %d, hit bounds %d-%d\n", i, mPuppetHolder->getSize(), Client::getMaxPlayerCount());
                    break;
                }

                if (!other->isConnected || !other->isInSameStage || !other->isIt || isYukimaru) {
                    continue;
                }

                if (other->gameMode != mMode && other->gameMode != GameMode::LEGACY) {
                    continue;
                }

                sead::Vector3f offset = sead::Vector3f(0.0f, 80.0f, 0.0f);
                float pupDist = vecDistance(other->playerPos + offset, al::getTrans(playerBase) + offset); // TODO: remove distance calculations and use hit sensors to determine this

                if (pupDist < 200.f && ((PlayerActorHakoniwa*)playerBase)->mDimKeeper->is2DModel == other->is2D) {
                    if (!PlayerFunction::isPlayerDeadStatus(playerBase)) {
                        GameDataFunction::killPlayer(GameDataHolderAccessor(this));
                        playerBase->startDemoPuppetable();
                        al::setVelocityZero(playerBase);
                        rs::faceToCamera(playerBase);
                        ((PlayerActorHakoniwa*)playerBase)->mPlayerAnimator->endSubAnim();
                        ((PlayerActorHakoniwa*)playerBase)->mPlayerAnimator->startAnimDead();

                        updateTagState(true);
                    }
                } else if (PlayerFunction::isPlayerDeadStatus(playerBase)) {
                    updateTagState(true);
                }
            }
        }

        mModeTimer->updateTimer();
    }

    // Gravity
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
        } else if (al::isPadTriggerZL(-1) && al::isPadTriggerLeft(-1)) {
            killMainPlayer(((PlayerActorHakoniwa*)playerBase));
        }
    }

    // Switch roles
    if (al::isPadTriggerUp(-1) && !al::isPadHoldZL(-1)) {
        updateTagState(!mInfo->mIsPlayerIt);
    }

    mInfo->mHidingTime = mModeTimer->getTime();
}

bool HideAndSeekMode::showNameTag(PuppetInfo* other) {
    return (
        (other->gameMode != mMode && other->gameMode != GameMode::LEGACY)
        || (isPlayerIt() && other->isIt)
    );
}

void HideAndSeekMode::debugMenuControls(sead::TextWriter* gTextWriter) {
    gTextWriter->printf("- L + ← | Enable/disable Hide & Seek [H&S]\n");
    gTextWriter->printf("- [H&S] ↑ | Switch between hider and seeker\n");
    gTextWriter->printf("- [H&S][Hider] ← | Decrease hiding time\n");
    gTextWriter->printf("- [H&S][Hider] → | Increase hiding time\n");
    gTextWriter->printf("- [H&S][Hider] L + ↓ | Reset hiding time\n");
    gTextWriter->printf("- [H&S][Gravity] L + → | Toggle gravity camera\n");
}

void HideAndSeekMode::updateTagState(bool isSeeking) {
    mInfo->mIsPlayerIt = isSeeking;

    if (isSeeking) {
        mModeTimer->disableTimer();
        mModeLayout->showSeeking();
    } else {
        mModeTimer->enableTimer();
        mModeLayout->showHiding();
        mInvulnTime = 0;
    }

    Client::sendGameModeInfPacket();
}

void HideAndSeekMode::onBorderPullBackFirstStep(al::LiveActor* actor) {
    if (isUseGravity()) {
        killMainPlayer(actor);
    }
}

void HideAndSeekMode::createCustomCameraTicket(al::CameraDirector* director) {
    mTicket = director->createCameraFromFactory("CameraPoserCustom", nullptr, 0, 5, sead::Matrix34f::ident);
}
