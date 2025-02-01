#include "server/hotpotato/HotPotatoMode.hpp"
#include <cmath>
#include "actors/PuppetActor.h"
#include "al/async/FunctorV0M.hpp"
#include "al/util.hpp"
#include "al/util/CameraUtil.h"
#include "al/util/ControllerUtil.h"
#include "al/util/LiveActorUtil.h"
#include "al/util/NerveUtil.h"
#include "cameras/CameraPoserActorSpectate.h"
#include "game/GameData/GameDataFunction.h"
#include "game/GameData/GameDataHolderAccessor.h"
#include "game/Layouts/CoinCounter.h"
#include "game/Layouts/MapMini.h"
#include "game/Player/HackCap.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Player/PlayerHitPointData.h"
#include "heap/seadHeapMgr.h"
#include "layouts/HotPotatoIcon.h"
#include "logger.hpp"
#include "math/seadVector.h"
#include "puppets/PuppetInfo.h"
#include "rs/util.hpp"
#include "server/hotpotato/HotHintArrow.h"
#include "server/hotpotato/HotPotatoScore.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/Client.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include <heap/seadHeap.h>
#include "server/gamemode/GameModeManager.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "rs/util/InputUtil.h"

#include "basis/seadNew.h"
#include "server/hotpotato/HotPotatoConfigMenu.hpp"

HotPotatoMode::HotPotatoMode(const char* name) : GameModeBase(name) {}

void HotPotatoMode::init(const GameModeInitInfo& info) {
    mSceneObjHolder = info.mSceneObjHolder;
    mMode = info.mMode;
    mCurScene = (StageScene*)info.mScene;
    mPuppetHolder = info.mPuppetHolder;

    GameModeInfoBase* curGameInfo = GameModeManager::instance()->getInfo<HotPotatoInfo>();

    if (curGameInfo) Logger::log("Gamemode info found: %s %s\n", GameModeFactory::getModeString(curGameInfo->mMode), GameModeFactory::getModeString(info.mMode));
    else Logger::log("No gamemode info found\n");
    if (curGameInfo && curGameInfo->mMode == mMode) {
        mInfo = (HotPotatoInfo*)curGameInfo;
        mModeTimer = new GameModeTimer(mInfo->mRoundTimer);
    } else {
        if (curGameInfo) delete curGameInfo;  // attempt to destory previous info before creating new one
        mInfo = GameModeManager::instance()->createModeInfo<HotPotatoInfo>();
        mModeTimer = new GameModeTimer();
    }

    mInfo->mRunnerPlayers.allocBuffer(0x10, al::getSceneHeap());
    mInfo->mChaserPlayers.allocBuffer(0x10, al::getSceneHeap());

    Logger::log("Scene Heap Free Size: %f/%f\n", al::getSceneHeap()->getFreeSize() * 0.001f, al::getSceneHeap()->getSize() * 0.001f);

    mModeLayout = new HotPotatoIcon("HotPotatoIcon", *info.mLayoutInitInfo);
    mInfo->mPlayerTagScore.setTargetLayout(mModeLayout);
    
    Logger::log("Scene Heap Free Size: %f/%f\n", al::getSceneHeap()->getFreeSize() * 0.001f, al::getSceneHeap()->getSize() * 0.001f);

    // Create main player's ice block
    mMainPlayerIceBlock = new HotPlayerBlock("MainPlayerBlock");
    mMainPlayerIceBlock->init(*info.mActorInitInfo);

    // Create hint arrow
    mHintArrow = new HotHintArrow("ChaserHintArrow");
    mHintArrow->init(*info.mActorInitInfo);
}

void HotPotatoMode::processPacket(Packet *packet) {
    HotPotatoPacket* frzPak = (HotPotatoPacket*)packet;
    PuppetInfo* curInfo = Client::findPuppetInfo(frzPak->mUserID, false);

    if (!curInfo)
        return;

    if (frzPak->updateType & HotUpdateType::HOTPLAYER) {
        tryScoreEvent(frzPak, curInfo);

        // When puppet transitioning from frozen to unfrozen, disable the fall off flag
        if(curInfo->isHotPotatoFreeze && !frzPak->isFreeze)
            curInfo->isHotPotatoFallenOff = false;

        curInfo->isHotPotatoRunner = frzPak->isRunner;
        curInfo->isHotPotatoFreeze = frzPak->isFreeze;
        curInfo->hotPotatoScore = frzPak->score;
    }

    if (frzPak->updateType & HotUpdateType::HOTROUNDSTART && !mInfo->mIsRound) {
        HotPotatoRoundPacket* roundPak = (HotPotatoRoundPacket*)frzPak;
        startRound(al::clamp(roundPak->roundTime, u8(2), u8(60))); // Start round if round not already started
    }

    if (frzPak->updateType & HotUpdateType::HOTROUNDCANCEL && mInfo->mIsRound)
        endRound(true); // Abort round early on receiving cancel packet

    if (frzPak->updateType & HotUpdateType::HOTFALLOFF && mInfo->mIsRound) {
        curInfo->isHotPotatoFallenOff = true;
        
        if(!mInfo->mIsPlayerRunner)
            mInfo->mPlayerTagScore.eventScoreFallOff();
    }
}

Packet* HotPotatoMode::createPacket() {
    HotPotatoPacket *packet = new HotPotatoPacket();
    
    packet->mUserID = Client::getClientId();
    packet->updateType = mNextUpdateType;

    if(packet->updateType != HotUpdateType::HOTROUNDSTART) {
        packet->isRunner = mInfo->mIsPlayerRunner;
        packet->isFreeze = mInfo->mIsPlayerFreeze;
        packet->score = mInfo->mPlayerTagScore.mScore;

        return packet;
    }

    HotPotatoRoundPacket* roundPak = (HotPotatoRoundPacket*)packet;
    roundPak->roundTime = u8(mInfo->mRoundLength);

    return roundPak;
}

void HotPotatoMode::sendHotPacket(HotUpdateType updateType) {
    mNextUpdateType = updateType;
    Client::sendGamemodePacket();
}

void HotPotatoMode::begin() {
    unpause();

    mInvulnTime = 0.f;
    mSpectateIndex = -1;
    mPrevSpectateIndex = -2;
    mIsScoreEventsValid = true;

    if(mInfo->mIsRound)
        mModeTimer->enableTimer();
    mModeTimer->disableControl();
    mModeTimer->setTimerDirection(false);

    PlayerHitPointData* hit = mCurScene->mHolder.mData->mGameDataFile->getPlayerHitPointData();
    hit->mCurrentHit = hit->getMaxCurrent();
    hit->mIsKidsMode = true;

    sendHotPacket(HotUpdateType::HOTPLAYER);

    GameModeBase::begin();

    mCurScene->mSceneLayout->end();
}


void HotPotatoMode::end() {
    pause();

    mInvulnTime = 0.f;
    mIsScoreEventsValid = false;
    mCurScene->mSceneLayout->start();
    
    if(!GameModeManager::instance()->isPaused()) {
        if(mInfo->mIsPlayerFreeze)
            trySetPlayerRunnerState(HotState::HOTALIVE);
        
        if(al::isAlive(mMainPlayerIceBlock) && !al::isNerve(mMainPlayerIceBlock, &nrvHotPlayerBlockDisappear)) {
            mMainPlayerIceBlock->end();
            trySetPostProcessingType(HotPostProcessingType::HOTPPDISABLED);
        }
    }

    GameModeBase::end();
}

void HotPotatoMode::pause() {
    GameModeBase::pause();

    mModeLayout->tryEnd();
}

void HotPotatoMode::unpause() {
    GameModeBase::unpause();

    mModeLayout->appear();
}

void HotPotatoMode::update() {
    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if(!player)
        return;
    
    // Update the mode timer
    mModeTimer->updateTimer();
    mModeTimer->disableControl();
    
    // Check for a decrease in the minute value (how survival time score is awarded)
    if((mInfo->mRoundTimer.mMinutes > mModeTimer->getTime().mMinutes) && mInfo->mIsPlayerRunner)
        mInfo->mPlayerTagScore.eventScoreSurvivalTime();

    mInfo->mRoundTimer = mModeTimer->getTime();
    if(mModeTimer->isEnabled()) {
        if(mModeTimer->getTimeCombined() <= 0.f)
            endRound(false);
    }

    //Create list of runner and chaser player indexs
    mInfo->mRunnerPlayers.clear();
    mInfo->mChaserPlayers.clear();

    for(int i = 0; i < mPuppetHolder->getSize(); i++) {
        PuppetInfo *curInfo = Client::getPuppetInfo(i);
        if(!curInfo->isConnected)
            continue;
            
        if(curInfo->isHotPotatoRunner)
            mInfo->mRunnerPlayers.pushBack(curInfo);
        else
            mInfo->mChaserPlayers.pushBack(curInfo);
    }

    //Verify you are never frozen on chaser team
    if(!mInfo->mIsPlayerRunner && mInfo->mIsPlayerFreeze)
        trySetPlayerRunnerState(HotState::HOTALIVE);
    
    mInvulnTime += Time::deltaTime;

    // Puppet checks
    float closePupDistance = 9999999.f;
    PuppetInfo* closePup = nullptr;

    if (mInfo->mIsRound) {
        if (mInvulnTime >= 3) {
            bool isPDead = PlayerFunction::isPlayerDeadStatus(player);
            bool isP2D = ((PlayerActorHakoniwa*)player)->mDimKeeper->is2D;

            for (size_t i = 0; i < mPuppetHolder->getSize(); i++) {
                PuppetInfo *curInfo = Client::getPuppetInfo(i);
                float pupDist = al::calcDistance(player, curInfo->playerPos);

                if(!curInfo->isConnected || !curInfo->isInSameStage)
                    continue;
                
                // If this puppet is the new closest, set the closest info to the current puppet
                if(pupDist < closePupDistance && curInfo->isHotPotatoRunner && !curInfo->isHotPotatoFreeze) {
                    closePupDistance = pupDist;
                    closePup = curInfo;
                }

                if(!mInfo->mIsPlayerRunner)
                    continue;

                // Check to invert the player's Runner state manually
if (!mInfo->mIsPlayerFreeze && pupDist < 250.f && isP2D == curInfo->is2D && !isPDead && !curInfo->isHotPotatoRunner) {
    mInfo->mIsPlayerRunner = !mInfo->mIsPlayerRunner;  // Directly invert the player's Runner state
}




                //Check for unfreeze
                float hotMinTime = al::clamp(3.f + (mInfo->mFreezeCount * 0.5f), 3.f, 7.f);
                if (mInvulnTime >= hotMinTime && mInfo->mIsPlayerFreeze && pupDist < 200.f && isP2D == curInfo->is2D
                && !isPDead && curInfo->isHotPotatoRunner && !curInfo->isHotPotatoFreeze) {
                    trySetPlayerRunnerState(HotState::HOTALIVE);
                }
            }
        }
    }

    // Set the target position to the closest puppet
    if(closePup)
        mHintArrow->setTarget(&closePup->playerPos);
    else
        mHintArrow->setTarget(nullptr);

    // Update recovery event timer
    if(mRecoveryEventFrames > 0) {
        mRecoveryEventFrames--;
        if(mRecoveryEventFrames == 0)
            tryEndRecoveryEvent();
    }

    // Update endgame event
    if(mIsEndgameActive) {
        mEndgameTimer += Time::deltaTime;
        if(mEndgameTimer > 6.f) {
            mInfo->mIsPlayerRunner = true;
            mInvulnTime = 0.f;
            sendHotPacket(HotUpdateType::HOTPLAYER);

            mIsEndgameActive = false;
            tryStartRecoveryEvent(true);
        }
    }

    // Update other players if your score changes
    HotPotatoScore* score = &mInfo->mPlayerTagScore;
    if(score->mScore != score->mPrevScore) {
        score->mPrevScore = score->mScore;
        sendHotPacket(HotUpdateType::HOTPLAYER);
    };

    // Main player's ice block state and post processing
    if(mInfo->mIsPlayerFreeze) {
        if(!al::isAlive(mMainPlayerIceBlock)) {
            mMainPlayerIceBlock->appear();
            trySetPostProcessingType(HotPostProcessingType::HOTPPFROZEN);
        }
        
        //Lock block onto player
        al::setTrans(mMainPlayerIceBlock, al::getTrans(player));
        al::setQuat(mMainPlayerIceBlock, al::getQuat(player));

    } else {
        if(al::isAlive(mMainPlayerIceBlock) && mMainPlayerIceBlock->mIsLocked) {
            mMainPlayerIceBlock->end();
            trySetPostProcessingType(HotPostProcessingType::HOTPPDISABLED);
        }
    }

    // D-Pad functions
    if (al::isPadTriggerUp(-1) && !al::isPadHoldL(-1)&& !al::isPadHoldZR(-1) && !al::isPadHoldR(-1)
    && !mInfo->mIsPlayerFreeze && mRecoveryEventFrames == 0 && !mIsEndgameActive && !mInfo->mIsRound) {
        mInfo->mIsPlayerRunner = !mInfo->mIsPlayerRunner;
        mInvulnTime = 0.f;

        sendHotPacket(HotUpdateType::HOTPLAYER);
    }

    if (al::isPadTriggerDown(-1) && al::isPadHoldL(-1) && !mInfo->mIsPlayerFreeze && mRecoveryEventFrames == 0 && !mIsEndgameActive)
        mInfo->mPlayerTagScore.resetScore();
    
    if (al::isPadTriggerUp(-1) && al::isPadHoldR(-1) && mInfo->mIsHostMode && !mInfo->mIsRound) {
        startRound(mInfo->mRoundLength);
        sendHotPacket(HotUpdateType::HOTROUNDSTART);
    }
    if (al::isPadTriggerDown(-1) && al::isPadHoldR(-1) && mInfo->mIsHostMode && mInfo->mIsRound) {
        endRound(true);
        sendHotPacket(HotUpdateType::HOTROUNDCANCEL);
    }

    //Debug freeze buttons
    if (mInfo->mIsDebugMode) {
        if (al::isPadTriggerRight(-1) && al::isPadHoldX(-1) && mInfo->mIsPlayerRunner)
            trySetPlayerRunnerState(HotState::HOTALIVE);
        if (al::isPadTriggerRight(-1) && al::isPadHoldY(-1) && mInfo->mIsPlayerRunner)
            trySetPlayerRunnerState(HotState::HOTFREEZE);
        if (al::isPadTriggerRight(-1) && al::isPadHoldA(-1))
            mInfo->mPlayerTagScore.eventScoreDebug();
        if (al::isPadTriggerRight(-1) && al::isPadHoldB(-1))
            tryStartEndgameEvent();
        if (al::isPadTriggerLeft(-1) && al::isPadHoldA(-1))
            mModeTimer->setTime(0.f, 5, 1, 0);
    }

    // Verify standard hud is hidden
    if(!mCurScene->mSceneLayout->isEnd())
        mCurScene->mSceneLayout->end();
    }
