#include "server/sardines/SardineMode.hpp"
#include "al/async/FunctorV0M.hpp"
#include "al/util.hpp"
#include "al/util/ControllerUtil.h"
#include "al/util/LiveActorUtil.h"
#include "al/util/MathUtil.h"
#include "game/GameData/GameDataFunction.h"
#include "game/GameData/GameDataHolderAccessor.h"
#include "game/Layouts/CoinCounter.h"
#include "game/Layouts/MapMini.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Player/PlayerFunction.h"
#include "heap/seadHeapMgr.h"
#include "logger.hpp"
#include "math/seadVector.h"
#include "rs/util.hpp"
#include "server/Client.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/sardines/SardineConfigMenu.hpp"
#include "server/sardines/SardinePacket.hpp"
#include <cmath>
#include <heap/seadHeap.h>

#include "basis/seadNew.h"

SardineMode::SardineMode(const char* name) : GameModeBase(name) {}

void SardineMode::init(const GameModeInitInfo& info) {
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
        mInfo = (SardineInfo*)curGameInfo;
        mModeTimer = new GameModeTimer(mInfo->mHidingTime);
        Logger::log("Reinitialized timer with time %d:%.2d\n", mInfo->mHidingTime.mMinutes, mInfo->mHidingTime.mSeconds);
    } else {
        if (curGameInfo) {
            delete curGameInfo; // attempt to destory previous info before creating new one
        }

        mInfo = GameModeManager::instance()->createModeInfo<SardineInfo>();

        mModeTimer = new GameModeTimer();
    }

    sead::ScopedCurrentHeapSetter heapSetterr(GameModeManager::getSceneHeap());

    mModeLayout = new SardineIcon("SardineIcon", *info.mLayoutInitInfo);
    mModeLayout->showSolo();
}

void SardineMode::processPacket(Packet* _packet) {
    SardinePacket*    packet     = (SardinePacket*)_packet;
    SardineUpdateType updateType = packet->updateType();

    // if the packet is for our player, edit info for our player
    if (packet->mUserID == Client::getClientId()) {
        if (updateType & SardineUpdateType::TIME) {
            mInfo->mHidingTime.mMilliseconds = 0.0;
            mInfo->mHidingTime.mSeconds      = packet->seconds;
            mInfo->mHidingTime.mMinutes      = packet->minutes % 60;
            mInfo->mHidingTime.mHours        = packet->minutes / 60;
            mModeTimer->setTime(mInfo->mHidingTime);
        }

        if (updateType & SardineUpdateType::STATE) {
            updateTagState(packet->isIt);
        } else if (updateType & SardineUpdateType::TIME) {
            Client::sendGameModeInfPacket();
        }

        return;
    }

    PuppetInfo* other = Client::findPuppetInfo(packet->mUserID, false);
    if (!other) {
        return;
    }

    if (updateType & SardineUpdateType::STATE) {
        other->isIt = packet->isIt;
    }

    if (updateType & SardineUpdateType::TIME) {
        other->seconds = packet->seconds;
        other->minutes = packet->minutes;
    }
}

Packet* SardineMode::createPacket() {
    if (!isModeActive()) {
        DisabledGameModeInf* packet = new DisabledGameModeInf(Client::getClientId());
        return packet;
    }

    SardinePacket* packet = new SardinePacket();
    packet->mUserID    = Client::getClientId();
    packet->isIt       = isPlayerIt();
    packet->seconds    = mInfo->mHidingTime.mSeconds;
    packet->minutes    = mInfo->mHidingTime.mMinutes + mInfo->mHidingTime.mHours * 60;
    packet->setUpdateType(static_cast<SardineUpdateType>(SardineUpdateType::STATE | SardineUpdateType::TIME));
    return packet;
}

void SardineMode::begin() {
    unpause();

    mIsFirstFrame = true;

    GameModeBase::begin();
}

void SardineMode::end() {
    pause();

    GameModeBase::end();
}

void SardineMode::pause() {
    GameModeBase::pause();

    mModeLayout->tryEnd();
    mModeTimer->disableTimer();
}

void SardineMode::unpause() {
    GameModeBase::unpause();

    mModeLayout->appear();

    if (mInfo->mIsIt) {
        mModeTimer->enableTimer();
        mModeLayout->showPack();
    } else {
        mModeTimer->disableTimer();
        mModeLayout->showSolo();
    }
}

void SardineMode::update() {
    PlayerActorBase* playerBase = rs::getPlayerActor(mCurScene);

    bool isYukimaru = !playerBase->getPlayerInfo(); // if PlayerInfo is a nullptr, that means we're dealing with the bound bowl racer

    float highPuppetDistance = -1.f;
    int farPuppetID = -1;
    bool isAnybodyElseIt = false;

    if (mIsFirstFrame) {
        if (mInfo->mIsUseGravityCam && mTicket) {
            al::startCamera(mCurScene, mTicket, -1);
        }
        mIsFirstFrame = false;
    }

    if (playerBase) {
        for (size_t i = 0; i < (size_t)mPuppetHolder->getSize(); i++) {
            PuppetInfo* other = Client::getPuppetInfo(i);
            if (!other) {
                Logger::log("Checking %d, hit bounds %d-%d\n", i, mPuppetHolder->getSize(), Client::getMaxPlayerCount());
                break;
            }

            if (!other->isConnected || !other->isIt) {
                continue;
            }

            if (other->gameMode != mMode && other->gameMode != GameMode::LEGACY) {
                continue;
            }

            isAnybodyElseIt = true;

            if (!other->isInSameStage) {
                continue;
            }

            float pupDist = al::calcDistance(playerBase, other->playerPos);

            if (highPuppetDistance < pupDist || highPuppetDistance == -1) {
                highPuppetDistance = pupDist;
                farPuppetID        = i;
            }

            if (
                !mInfo->mIsIt
                && !isYukimaru
                && pupDist < 300.0f
                && other->is2D == ((PlayerActorHakoniwa*)playerBase)->mDimKeeper->is2DModel
                && !PlayerFunction::isPlayerDeadStatus(playerBase)
            ) {
                updateTagState(true);
            }
        }
    }

    mModeTimer->updateTimer();

    // Tin detaching
    if (mInfo->mIsIt && (
        PlayerFunction::isPlayerDeadStatus(playerBase)
        || (
            mInfo->mIsTether
            && mInfo->mIsTetherSnap
            && pullDistanceMax < highPuppetDistance
        )
    )) {
        updateTagState(false);
    }

    // Player pulling
    if (pullDistanceMin <= highPuppetDistance && mInfo->mIsIt && farPuppetID != -1 && mInfo->mIsTether) {
        sead::Vector3f  target    = Client::getPuppetInfo(farPuppetID)->playerPos;
        sead::Vector3f* playerPos = al::getTransPtr(playerBase);
        sead::Vector3f  direction = target - *playerPos;

        al::normalize(&direction);

        playerPos->add(direction * ((al::calcDistance(playerBase, target) - pullDistanceMin) / pullPowerRate));
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
        updateTagState(!mInfo->mIsIt && !isAnybodyElseIt);
    }

    mInfo->mHidingTime = mModeTimer->getTime();
}

bool SardineMode::showNameTag(PuppetInfo* other) {
    return (
        (other->gameMode != mMode && other->gameMode != GameMode::LEGACY)
        || (isPlayerIt() && other->isIt)
    );
}

void SardineMode::debugMenuControls(sead::TextWriter* gTextWriter) {
    gTextWriter->printf("- L + ← | Enable/disable Sardines [S]\n");
    gTextWriter->printf("- [S] ↑ | Switch between sardine and pack\n");
    gTextWriter->printf("- [S] ← | Decrease pack time\n");
    gTextWriter->printf("- [S] → | Increase pack time\n");
    gTextWriter->printf("- [S] L + ↓ | Reset pack time\n");
    gTextWriter->printf("- [S][Gravity] L + → | Toggle gravity camera\n");
}

void SardineMode::updateTagState(bool isIt) {
    mInfo->mIsIt = isIt;

    if (isIt) {
        mModeTimer->enableTimer();
        mModeLayout->showPack();
    } else {
        mModeTimer->disableTimer();
        mModeLayout->showSolo();
    }

    Client::sendGameModeInfPacket();
}

void SardineMode::onBorderPullBackFirstStep(al::LiveActor* actor) {
    if (isUseGravity()) {
        killMainPlayer(actor);
    }
}

void SardineMode::createCustomCameraTicket(al::CameraDirector* director) {
    mTicket = director->createCameraFromFactory("CameraPoserCustom", nullptr, 0, 5, sead::Matrix34f::ident);
}
