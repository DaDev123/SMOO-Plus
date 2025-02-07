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
#include "layouts/HideAndSeekIcon.h"
#include "logger.hpp"
#include "math/seadVector.h"
#include "packets/Packet.h"
#include "rs/util.hpp"
#include "rs/util/PlayerUtil.h"
#include "server/gamemode/GameModeBase.hpp"
#include "server/Client.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include <heap/seadHeap.h>
#include <math.h>
#include "server/gamemode/GameModeManager.hpp"
#include "server/gamemode/GameModeFactory.hpp"

#include "basis/seadNew.h"
#include "server/hns/HideAndSeekConfigMenu.hpp"

//Spectator Files
#include "cameras/CameraPoserActorSpectate.h"

HideAndSeekMode::HideAndSeekMode(const char* name) : GameModeBase(name) {}

void HideAndSeekMode::init(const GameModeInitInfo& info) {
    mSceneObjHolder = info.mSceneObjHolder;
    mMode = info.mMode;
    mCurScene = (StageScene*)info.mScene;
    mPuppetHolder = info.mPuppetHolder;

    GameModeInfoBase* curGameInfo = GameModeManager::instance()->getInfo<HideAndSeekInfo>();

    if (curGameInfo) Logger::log("Gamemode info found: %s %s\n", GameModeFactory::getModeString(curGameInfo->mMode), GameModeFactory::getModeString(info.mMode));
    else Logger::log("No gamemode info found\n");
    if (curGameInfo && curGameInfo->mMode == mMode) {
        mInfo = (HideAndSeekInfo*)curGameInfo;
        mModeTimer = new GameModeTimer(mInfo->mHidingTime);
        Logger::log("Reinitialized timer with time %d:%.2d\n", mInfo->mHidingTime.mMinutes, mInfo->mHidingTime.mSeconds);
    } else {
        if (curGameInfo) delete curGameInfo;  // attempt to destory previous info before creating new one
        
        mInfo = GameModeManager::instance()->createModeInfo<HideAndSeekInfo>();
        
        mModeTimer = new GameModeTimer();
    }

    mModeLayout = new HideAndSeekIcon("HideAndSeekIcon", *info.mLayoutInitInfo);

    mModeLayout->showSeeking();

    mModeTimer->disableTimer();

}

void HideAndSeekMode::processPacket(Packet *packet) {
    HideAndSeekPacket* tagPacket = (HideAndSeekPacket*)packet;

    // if the packet is for our player, edit info for our player
    if (tagPacket->mUserID == Client::getClientId() && GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK)) {

        HideAndSeekMode* mode = GameModeManager::instance()->getMode<HideAndSeekMode>();
        HideAndSeekInfo* curInfo = GameModeManager::instance()->getInfo<HideAndSeekInfo>();

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

Packet *HideAndSeekMode::createPacket() {

    HideAndSeekPacket *packet = new HideAndSeekPacket();

    packet->mUserID = Client::getClientId();

    packet->isIt = isPlayerIt();

    packet->minutes = mInfo->mHidingTime.mMinutes;
    packet->seconds = mInfo->mHidingTime.mSeconds;
    packet->updateType = static_cast<TagUpdateType>(TagUpdateType::STATE | TagUpdateType::TIME);

    return packet;
}

void HideAndSeekMode::begin() {

    unpause();

    mIsFirstFrame = true;
    
    mInvulnTime = 0.0f;
    mSpectateIndex = -1;

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
    
    if (!mInfo->mIsPlayerIt) {
        mModeTimer->enableTimer();
        mModeLayout->showHiding();
    } else {
        mModeTimer->disableTimer();
        mModeLayout->showSeeking();
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
            
                        float pupDist = al::calcDistance(playerBase, curInfo->playerPos); // TODO: remove distance calculations and use hit sensors to determine this

                        if (!isYukimaru) {
                            if(pupDist < 200.f && ((PlayerActorHakoniwa*)playerBase)->mDimKeeper->is2DModel == curInfo->is2D) {
                                if(!PlayerFunction::isPlayerDeadStatus(playerBase)) {
                                    
                                    GameDataFunction::killPlayer(GameDataHolderAccessor(this));
                                    playerBase->startDemoPuppetable();
                                    al::setVelocityZero(playerBase);
                                    rs::faceToCamera(playerBase);
                                    ((PlayerActorHakoniwa*)playerBase)->mPlayerAnimator->endSubAnim();
                                    ((PlayerActorHakoniwa*)playerBase)->mPlayerAnimator->startAnimDead();

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

    if (al::isPadTriggerUp(-1) && !al::isPadHoldR(-1))
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

// Check if R and D-pad Left are pressed at the same time
if (al::isPadHoldR(-1) && al::isPadTriggerUp(-1)) {
    if (!mTicket->mIsActive && mInfo->mIsPlayerIt) {
        al::startCamera(mCurScene, mTicket, -1);  // Activate the camera
    } else if (mTicket->mIsActive) {
        al::endCamera(mCurScene, mTicket, 0, false);  // Deactivate the camera
    }
}

// Existing Spectate Camera logic
if (mTicket->mIsActive && mInfo->mIsPlayerIt) {
    updateSpectateCam(playerBase);
}

}


void HideAndSeekMode::updateSpectateCam(PlayerActorBase* playerBase)
{
    //If the specate camera ticket is active, get the camera poser
    al::CameraPoser* curPoser;
    al::CameraDirector* director = mCurScene->getCameraDirector();
    if (director) {
        al::CameraPoseUpdater* updater = director->getPoseUpdater(0);
        if (updater && updater->mTicket) {
            curPoser = updater->mTicket->mPoser;
        }
    }
    
    //Verify 100% that this poser is the actor spectator
    if (al::isEqualString(curPoser->getName(), "CameraPoserActorSpectate")) {
        cc::CameraPoserActorSpectate* spectatePoser = (cc::CameraPoserActorSpectate*)curPoser;
        spectatePoser->setPlayer(playerBase);
        //Increase or decrease spectate index, followed by clamping it
        int indexDirection = 0;
        if(al::isPadTriggerRight(-1)) indexDirection = 1; //Move index right
        if(al::isPadTriggerLeft(-1)) indexDirection = -1; //Move index left
        //Force index to increase if your current target changes stages
        if(mSpectateIndex != -1)
            if(!mInfo->isIt.at(mSpectateIndex)->isInSameStage)
                indexDirection = 1; //Move index right
        //Loop over indexs until you find a sutible one in the same stage
        bool isFinalIndex = false;
        while(!isFinalIndex) {
            mSpectateIndex += indexDirection;
            // Start by clamping the index
            if(mSpectateIndex < -1) mSpectateIndex = mInfo->isIt.size() - 1;
            if(mSpectateIndex >= mInfo->isIt.size()) mSpectateIndex = -1;
            // If not in same stage, skip
            if(mSpectateIndex != -1) {
                if(mInfo->isIt.at(mSpectateIndex)->isInSameStage)
                    isFinalIndex = true;
            } else {
                isFinalIndex = true;
            }
        }
        
        //If no index change is happening, end here
        if(mPrevSpectateIndex == mSpectateIndex)
            return;
        //Apply index to target actor and HUD
        if(mSpectateIndex == -1) {
            spectatePoser->setTargetActor(al::getTransPtr(playerBase));
        } else {
            spectatePoser->setTargetActor(&mInfo->isIt.at(mSpectateIndex)->playerPos);
        }
        mPrevSpectateIndex = mSpectateIndex;
    }
}




// Hooks

namespace al {
    class Triangle;
    bool isFloorCode(al::Triangle const&,char const*);
}