#include "server/inf/InfectionMode.hpp"
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
#include "layouts/InfectionIcon.h"
#include "logger.hpp"
#include "math/seadVector.h"
#include "packets/Packet.h"
#include "rs/util.hpp"
#include "rs/util/PlayerUtil.h"
#include "server/gamemode/GameModeBase.hpp"
#include "server/Client.hpp"
#include <heap/seadHeap.h>
#include <math.h>
#include "server/gamemode/GameModeManager.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "server/hns/HideAndSeekMode.hpp"

#include "basis/seadNew.h"
#include "server/inf/InfectionConfigMenu.hpp"


InfectionMode::InfectionMode(const char* name) : GameModeBase(name) {}

void InfectionMode::init(const GameModeInitInfo& info) {
    mSceneObjHolder = info.mSceneObjHolder;
    mMode = info.mMode;
    mCurScene = (StageScene*)info.mScene;
    mPuppetHolder = info.mPuppetHolder;

    GameModeInfoBase* curGameInfo = GameModeManager::instance()->getInfo<InfectionInfo>();

    if (curGameInfo) Logger::log("Gamemode info found: %s %s\n", GameModeFactory::getModeString(curGameInfo->mMode), GameModeFactory::getModeString(info.mMode));
    else Logger::log("No gamemode info found\n");
    if (curGameInfo && curGameInfo->mMode == mMode) {
        mInfo = (InfectionInfo*)curGameInfo;
        mModeTimer = new GameModeTimer(mInfo->mHidingTime);
        Logger::log("Reinitialized timer with time %d:%.2d\n", mInfo->mHidingTime.mMinutes, mInfo->mHidingTime.mSeconds);
    } else {
        if (curGameInfo) delete curGameInfo;  // attempt to destory previous info before creating new one
        
        mInfo = GameModeManager::instance()->createModeInfo<InfectionInfo>();
        
        mModeTimer = new GameModeTimer();
    }

    mModeLayout = new InfectionIcon("InfectionIcon", *info.mLayoutInitInfo);

    mModeLayout->showSeeking();

    mModeTimer->disableTimer();

}

void InfectionMode::processPacket(Packet *packet) {
    InfectionPacket* tagPacket = (InfectionPacket*)packet;

    // if the packet is for our player, edit info for our player
    if (tagPacket->mUserID == Client::getClientId() && GameModeManager::instance()->isMode(GameMode::Infection)) {

        InfectionMode* mode = GameModeManager::instance()->getMode<InfectionMode>();
        InfectionInfo* curInfo = GameModeManager::instance()->getInfo<InfectionInfo>();

        if (tagPacket->updateType & TagUpdateType::STATE) {
            mode->setPlayerTagState(tagPacket->isIt);
        }

        if (tagPacket->updateType & TagUpdateType::TIME) {
            curInfo->mHidingTime.mSeconds = tagPacket->seconds;
            curInfo->mHidingTime.mMinutes = tagPacket->minutes;
        }

        return;

    }

    PuppetInfo* curInfo = Client::findPuppetInfo(tagPacket->mUserID, false);

    if (!curInfo) {
        return;
    }

    curInfo->isIt = tagPacket->isIt;
    curInfo->seconds = tagPacket->seconds;
    curInfo->minutes = tagPacket->minutes;
}

Packet *InfectionMode::createPacket() {

    InfectionPacket *packet = new InfectionPacket();

    packet->mUserID = Client::getClientId();

    packet->isIt = isPlayerIt();

    packet->minutes = mInfo->mHidingTime.mMinutes;
    packet->seconds = mInfo->mHidingTime.mSeconds;
    packet->updateType = static_cast<TagUpdateType>(TagUpdateType::STATE | TagUpdateType::TIME);

    return packet;
}

void InfectionMode::begin() {

    unpause();

    mIsFirstFrame = true;
    
    mInvulnTime = 0.0f;

    GameModeBase::begin();
}


void InfectionMode::end() {

    pause();

    GameModeBase::end();
}

void InfectionMode::pause() {
    GameModeBase::pause();

    mModeLayout->tryEnd();
    mModeTimer->disableTimer();
}

void InfectionMode::unpause() {
    GameModeBase::unpause();

    mModeLayout->appear();
    
    if (!mInfo->mIsPlayerIt) {
        mModeTimer->enableTimer();
        mModeLayout->showHiding();
    } else {
        mModeTimer->disableTimer();
        mModeLayout->showSeeking();
    }
}

bool isInInfectAnim = false;

void InfectionMode::update() {

    PlayerActorBase* playerBase = rs::getPlayerActor(mCurScene);

    if(isInInfectAnim && ((PlayerActorHakoniwa*)playerBase)->mPlayerAnimator->isSubAnimEnd()){
        playerBase->endDemoPuppetable();
        isInInfectAnim = false;
    }

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
    if (!mInfo->mIsPlayerIt) {
        if (mInvulnTime >= 5) {  

            if (playerBase) {
                for (size_t i = 0; i < mPuppetHolder->getSize(); i++)
                {
                    PuppetInfo *curInfo = Client::getPuppetInfo(i);

                    if (!curInfo) {
                        Logger::log("Checking %d, hit bounds %d-%d\n", i, mPuppetHolder->getSize(), Client::getMaxPlayerCount());
                        break;
                    }

                    if(curInfo->isConnected && curInfo->isInSameStage && curInfo->isIt) {

                        sead::Vector3f offset = sead::Vector3f(0.0f, 80.0f, 0.0f);
            
                        float pupDist = vecDistance(curInfo->playerPos + offset, al::getTrans(playerBase) + offset); // TODO: remove distance calculations and use hit sensors to determine this

                        if (!isYukimaru) {
                            if(pupDist < 200.f && ((PlayerActorHakoniwa*)playerBase)->mDimKeeper->is2DModel == curInfo->is2D) {
                                if(!PlayerFunction::isPlayerDeadStatus(playerBase) && !rs::isActiveDemoPlayerPuppetable(playerBase)) {

                                    playerBase->startDemoPuppetable();
                                    al::setVelocityZero(playerBase);
                                    rs::faceToCamera(playerBase);
                                    ((PlayerActorHakoniwa*)playerBase)->mPlayerAnimator->endSubAnim();
                                    ((PlayerActorHakoniwa*)playerBase)->mPlayerAnimator->startSubAnim("DeadSand");
                                    isInInfectAnim = true;
                                    
                                    

                                
                                    mInfo->mIsPlayerIt = true;
                                    mModeTimer->disableTimer();
                                    mModeLayout->showSeeking();
                                    
                                    Client::sendGamemodePacket();
                                }
                            } else if (PlayerFunction::isPlayerDeadStatus(playerBase)) {

                                mInfo->mIsPlayerIt = true;
                                mModeTimer->disableTimer();
                                mModeLayout->showSeeking();

                                Client::sendGamemodePacket();
                                
                            }
                        }
                    }
                }
            }
        }else {
            mInvulnTime += Time::deltaTime;
        }

        mModeTimer->updateTimer();
        
    } else {
        mModeTimer->timerControl();
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

    if (al::isPadTriggerUp(-1) && !al::isPadHoldZL(-1))
    {
        mInfo->mIsPlayerIt = !mInfo->mIsPlayerIt;

        mModeTimer->toggleTimer();

        if(!mInfo->mIsPlayerIt) {
            mInvulnTime = 0;
            mModeLayout->showHiding();
        } else {
            mModeLayout->showSeeking();
        }

        Client::sendGamemodePacket();
    }

    mInfo->mHidingTime = mModeTimer->getTime();
}