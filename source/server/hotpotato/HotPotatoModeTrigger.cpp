#include "al/util.hpp"
#include "al/util/LiveActorUtil.h"
#include "al/util/RandomUtil.h"
#include "math/seadVector.h"
#include "puppets/PuppetInfo.h"
#include "server/hotpotato/HotPotatoMode.hpp"
#include "al/alCollisionUtil.h"

/*
    ROUND START AND END FUNCTIONS
*/

void HotPotatoMode::startRound(int roundMinutes) {
    mInfo->mIsRound = true;
    mInfo->mFreezeCount = 0;

    mModeTimer->enableTimer();
    mModeTimer->disableControl();
    mModeTimer->setTimerDirection(false);

    // Starts at the round minutes - 1 (and 59 seconds to not instantly set off score event)
    mModeTimer->setTime(0.f, 59, roundMinutes-1, 0);
}

void HotPotatoMode::endRound(bool isAbort) {
    mInfo->mIsRound = false;
    mInfo->mFreezeCount = 0;

    mModeTimer->disableTimer();

    if(!mIsEndgameActive) {
        if(!mInfo->mIsPlayerRunner) {
            mInfo->mIsPlayerRunner = true;
            sendHotPacket(HotUpdateType::HOTPLAYER);
            return;
        }

        if(!isAbort)
            mInfo->mPlayerTagScore.eventScoreRunnerWin();

        if(mInfo->mIsPlayerFreeze)
            trySetPlayerRunnerState(HotState::HOTALIVE);
    }
}

/*
    SET THE RUNNER PLAYER'S FROZEN/ALIVE STATE
*/

bool HotPotatoMode::trySetPlayerRunnerState(HotState newState)
{
    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if(!player)
        return false;

    if(mInfo->mIsPlayerFreeze == newState || !mInfo->mIsPlayerRunner)
        return false;
    
    HackCap* hackCap = player->mHackCap;

    mInvulnTime = 0.f;
    
    if(newState == HotState::HOTALIVE) {
        mInfo->mIsPlayerFreeze = HotState::HOTALIVE;
        player->endDemoPuppetable();
    } else {
        if(!mInfo->mIsRound)
            return false;

        mInfo->mIsPlayerFreeze = HotState::HOTFREEZE;
        if(player->getPlayerHackKeeper()->currentHackActor)
            player->getPlayerHackKeeper()->cancelHackArea();
            
        player->startDemoPuppetable();
        player->mPlayerAnimator->endSubAnim();
        player->mPlayerAnimator->startAnim("DeadIce");

        hackCap->forcePutOn();

        mSpectateIndex = -1;
        mInfo->mFreezeCount++;

        if(isAllRunnerFrozen(nullptr))
            tryStartEndgameEvent();
    }

    sendHotPacket(HotUpdateType::HOTPLAYER);

    return true;
}

/*
    UPDATE PLAYER SCORES
    FUNCTION CALLED FROM client.cpp ON RECEIVING FREEZE TAG PACKETS
*/

void HotPotatoMode::tryScoreEvent(HotPotatoPacket* incomingPacket, PuppetInfo* sourcePuppet)
{
    if(!mCurScene || !sourcePuppet || !GameModeManager::instance()->isModeAndActive(GameMode::HOTPOTATO))
        return;
    
    if(!mCurScene->mIsAlive)
        return;

    // Get the distance of the incoming player
    PlayerActorBase* playerBase = rs::getPlayerActor(mCurScene);
    if(!playerBase)
        return;
    
    float puppetDistance = al::calcDistance(playerBase, sourcePuppet->playerPos);
    bool isInRange = puppetDistance < 600.f; // Only apply this score event if player is less than this many units away

    if(isInRange) {
        //Check for unfreeze score event
        if((mInfo->mIsPlayerRunner && !mInfo->mIsPlayerFreeze) && (sourcePuppet->isHotPotatoFreeze && !incomingPacket->isFreeze)) {
            //Verify that the target puppet wasn't frozen via falling off the map
            if(!sourcePuppet->isHotPotatoFallenOff)
                mInfo->mPlayerTagScore.eventScoreUnfreeze();
        }

        //Check for freeze score event
        if((!mInfo->mIsPlayerRunner) && (!sourcePuppet->isHotPotatoFreeze && incomingPacket->isFreeze)) {
            mInfo->mPlayerTagScore.eventScoreFreeze();
        }
    }

    // Checks if every runner is frozen, starts endgame sequence if so
    if(!sourcePuppet->isHotPotatoFreeze && incomingPacket->isFreeze && isAllRunnerFrozen(sourcePuppet)) {
        tryStartEndgameEvent();
    }
}

/*
    HANDLE PLAYER RECOVERY
    Player recovery is started by entering a death area or an endgame (wipeout)
*/

bool HotPotatoMode::tryStartRecoveryEvent(bool isEndgame)
{
    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if(!player)
        return false;

    if(mRecoveryEventFrames > 0 || !mWipeHolder)
        return false; //Something isn't applicable here, return fail
    
    mRecoveryEventFrames = (mRecoveryEventLength / 2) * (isEndgame + 1);
    mWipeHolder->startClose("FadeBlack", (mRecoveryEventLength / 4) * (isEndgame + 1));

    if(!isEndgame) {
        mRecoverySafetyPoint = player->mPlayerRecoverySafetyPoint->mSafetyPointPos;
        if(mInfo->mIsPlayerRunner && mInfo->mIsRound)
            sendHotPacket(HotUpdateType::HOTFALLOFF);
    } else {
        mRecoverySafetyPoint = sead::Vector3f::zero;
    }
    
    Logger::log("Recovery event %.00fx %.00fy %.00fz\n", mRecoverySafetyPoint.x, mRecoverySafetyPoint.y, mRecoverySafetyPoint.z);

    return true;
}

bool HotPotatoMode::tryEndRecoveryEvent()
{
    if(!mWipeHolder)
        return false; //Recovery event is already started, return fail
    
    mWipeHolder->startOpen(mRecoveryEventLength / 2);
    
    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if(!player)
        return false;

    // Set the player to frozen if they are a runner AND they had a valid recovery point
    if(mInfo->mIsPlayerRunner && mRecoverySafetyPoint != sead::Vector3f::zero && mInfo->mIsRound) {
        trySetPlayerRunnerState(HotState::HOTFREEZE);
        warpToRecoveryPoint(player);
    } else {
        trySetPlayerRunnerState(HotState::HOTALIVE);
        trySetPostProcessingType(HotPostProcessingType::HOTPPDISABLED);
    }

    // If player is a chaser with a valid recovery point, teleport (and disable collisions)
    if(!mInfo->mIsPlayerRunner || !mInfo->mIsRound) {
        player->startDemoPuppetable();
        if(mRecoverySafetyPoint != sead::Vector3f::zero)
            warpToRecoveryPoint(player);
        
        trySetPostProcessingType(HotPostProcessingType::HOTPPDISABLED);
    }

    // If player is being made alive, force end demo puppet state
    if(!mInfo->mIsPlayerFreeze)
        player->endDemoPuppetable();

    if(!mIsEndgameActive)
        mModeLayout->hideEndgameScreen();

    return true;
}

void HotPotatoMode::warpToRecoveryPoint(al::LiveActor *actor)
{
    if(mInfo->mChaserPlayers.size() == 0 || !mInfo->mIsPlayerRunner || !mInfo->mIsRound) {
        al::setTrans(actor, mRecoverySafetyPoint);
        return;
    }

    PuppetInfo* inf = mInfo->mChaserPlayers.at(al::getRandom(mInfo->mChaserPlayers.size()));
    if(!inf->isInSameStage || !inf->isConnected) {
        al::setTrans(actor, mRecoverySafetyPoint);
        return;
    }

    al::setTrans(actor, inf->playerPos);
}

/*
    START AN ENDGAME EVENT (wipeout)
*/

void HotPotatoMode::tryStartEndgameEvent()
{
    mIsEndgameActive = true;
    mEndgameTimer = 0.f;
    mModeLayout->showEndgameScreen();

    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if(!player)
        return;
    
    if(player->getPlayerHackKeeper()->currentHackActor)
        player->getPlayerHackKeeper()->cancelHackArea();
        
    player->startDemoPuppetable();
    rs::faceToCamera(player);
    player->mPlayerAnimator->endSubAnim();
    if(mInfo->mIsPlayerRunner) {
        player->mPlayerAnimator->startAnim("RaceResultLose");
        trySetPostProcessingType(HotPostProcessingType::HOTPPENDGAMELOSE);
    } else {
        player->mPlayerAnimator->startAnim("RaceResultWin");
        trySetPostProcessingType(HotPostProcessingType::HOTPPENDGAMEWIN);
        mInfo->mPlayerTagScore.eventScoreWipeout();
    }
    
    endRound(false);
}

/*
    SET THE POST PROCESSING STYLE IN THE GAMEMODE
*/

bool HotPotatoMode::trySetPostProcessingType(HotPostProcessingType type)
{
    u8 ppIdx = type;
    u32 curIdx = al::getPostProcessingFilterPresetId(mCurScene);

    if(ppIdx == curIdx || !mCurScene)
        return false; // Already set to target post processing type or doesn't have scene, return fail
    
    if(type == HotPostProcessingType::HOTPPDISABLED) {
        while(curIdx != ppIdx) {
            al::incrementPostProcessingFilterPreset(mCurScene);
            curIdx = (curIdx + 1) % 18;
        }

        al::invalidatePostProcessingFilter(mCurScene);
        return true; // Disabled current post processing mode
    }

    while(curIdx != ppIdx) {
        al::incrementPostProcessingFilterPreset(mCurScene);
        curIdx = (curIdx + 1) % 18;
    }

    al::validatePostProcessingFilter(mCurScene);

    Logger::log("Set post processing to %i\n", al::getPostProcessingFilterPresetId(mCurScene));

    return true; // Set post processing mode to on at desired index
}