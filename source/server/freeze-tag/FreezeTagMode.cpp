#include "server/freeze-tag/FreezeTagMode.hpp"
#include <cmath>
#include <stdint.h>
#include "heap/seadHeapMgr.h"
#include <heap/seadHeap.h>
#include "basis/seadNew.h"
#include "actors/PuppetActor.h"
#include "al/async/FunctorV0M.hpp"
#include "al/util.hpp"
#include "al/util/CameraUtil.h"
#include "al/util/ControllerUtil.h"
#include "al/util/LiveActorUtil.h"
#include "al/util/NerveUtil.h"
#include "rs/util/InputUtil.h"
#include "game/Player/PlayerHitPointData.h"
#include "game/StageScene/StageScene.h"
#include "logger.hpp"
#include "server/Client.hpp"
#include "server/gamemode/GameMode.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "server/freeze-tag/FreezeTagConfigMenu.hpp"
#include "server/freeze-tag/FreezeTagScore.hpp"
#include "server/freeze-tag/FreezeHintArrow.h"
#include "server/freeze-tag/FreezePlayerBlock.h"
#include "server/freeze-tag/FreezeTagIcon.h"

FreezeTagMode::FreezeTagMode(const char* name) : GameModeBase(name) {}

void FreezeTagMode::init(const GameModeInitInfo& info) {
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
        mInfo = (FreezeTagInfo*)curGameInfo;
        mModeTimer = new GameModeTimer(mInfo->mRoundTimer);
        Logger::log("Reinitialized timer with time %d:%.2d\n", mInfo->mRoundTimer.mMinutes, mInfo->mRoundTimer.mSeconds);
    } else {
        if (curGameInfo) {
            delete curGameInfo; // attempt to destory previous info before creating new one
        }

        mInfo = GameModeManager::instance()->createModeInfo<FreezeTagInfo>();

        mModeTimer = new GameModeTimer();
    }

    sead::ScopedCurrentHeapSetter heapSetterr(GameModeManager::getSceneHeap());

    mModeLayout = new FreezeTagIcon("FreezeTagIcon", *info.mLayoutInitInfo);
    mInfo->mPlayerTagScore.setTargetLayout(mModeLayout);

    mInfo->mRunnerPlayers.allocBuffer(0x10, al::getSceneHeap());
    mInfo->mChaserPlayers.allocBuffer(0x10, al::getSceneHeap());

    // Create main player's ice block
    mMainPlayerIceBlock = new FreezePlayerBlock("MainPlayerBlock");
    mMainPlayerIceBlock->init(*info.mActorInitInfo);

    // Create hint arrow
    mHintArrow = new FreezeHintArrow("ChaserHintArrow");
    mHintArrow->init(*info.mActorInitInfo);
}

void FreezeTagMode::processPacket(Packet* _packet) {
    FreezeTagPacket* packet     = (FreezeTagPacket*)_packet;
    FreezeUpdateType updateType = packet->updateType();

    /**
     * Ignore legacy game mode packets for other game modes
     *
     * Legacy Freeze-Tag packets that we are interested in should have been automatically
     * transformed from LEGACY to FREEZETAG by the logic in the gameMode() function.
     */
    if (packet->gameMode() == GameMode::LEGACY) {
        return;
    }

    PuppetInfo* other = Client::findPuppetInfo(packet->mUserID, false);
    if (!other) {
        return;
    }

    if (updateType == FreezeUpdateType::PLAYER) {
        tryScoreEvent(packet, other);

        // When puppet transitioning from frozen to unfrozen, disable the fall off flag
        if (other->isFreezeTagFreeze && !packet->isFreeze) {
            other->isFreezeTagFallenOff = false;
        }

        other->isFreezeTagRunner = packet->isRunner;
        other->isFreezeTagFreeze = packet->isFreeze;
        other->freezeTagScore    = packet->score;
    }

    if (mInfo->mIsRound) {
        if (updateType == FreezeUpdateType::ROUNDCANCEL) {
            endRound(true); // Abort round early on receiving cancel packet
        }

        if (updateType == FreezeUpdateType::FALLOFF) {
            other->isFreezeTagFallenOff = true;

            if (!mInfo->mIsPlayerRunner) {
                mInfo->mPlayerTagScore.eventScoreFallOff();
            }
        }
    } else if (updateType == FreezeUpdateType::ROUNDSTART) {
        FreezeTagRoundPacket* roundPacket = (FreezeTagRoundPacket*)packet;
        startRound(al::clamp(roundPacket->roundTime, u8(2), u8(60))); // Start round if round not already started
    }
}

Packet* FreezeTagMode::createPacket() {
    if (!isModeActive()) {
        DisabledGameModeInf* packet = new DisabledGameModeInf(Client::getClientId());
        return packet;
    }

    if (mNextUpdateType == FreezeUpdateType::ROUNDSTART) {
        FreezeTagRoundPacket* packet = new FreezeTagRoundPacket();
        packet->mUserID   = Client::getClientId();
        packet->roundTime = u8(mInfo->mRoundLength);
        packet->setUpdateType(FreezeUpdateType::ROUNDSTART);
        return packet;
    }

    FreezeTagPacket* packet = new FreezeTagPacket();
    packet->mUserID  = Client::getClientId();
    packet->isRunner = mInfo->mIsPlayerRunner;
    packet->isFreeze = mInfo->mIsPlayerFreeze;
    packet->score    = mInfo->mPlayerTagScore.mScore;
    packet->setUpdateType(mNextUpdateType);

    return packet;
}

void FreezeTagMode::begin() {
    unpause();

    mInvulnTime         = 0.f;
    mSpectateIndex      = -1; // ourself
    mPrevSpectateIndex  = -2;
    mIsScoreEventsValid = true;

    if (mInfo->mIsRound) {
        mModeTimer->enableTimer();
    }
    mModeTimer->disableControl();
    mModeTimer->setTimerDirection(false);

    PlayerHitPointData* hit = mCurScene->mHolder.mData->mGameDataFile->getPlayerHitPointData();
    hit->mCurrentHit = hit->getMaxCurrent();
    hit->mIsKidsMode = true;

    GameModeBase::begin();

    mCurScene->mSceneLayout->end();
}

void FreezeTagMode::end() {
    pause();

    mInvulnTime         = 0.f;
    mIsScoreEventsValid = false;

    mCurScene->mSceneLayout->start();

    if (!GameModeManager::instance()->isPaused()) {
        if (mInfo->mIsPlayerFreeze) {
            trySetPlayerRunnerState(FreezeState::ALIVE);
        }

        if (mTicket->mIsActive) {
            al::endCamera(mCurScene, mTicket, 0, false);
        }

        if (al::isAlive(mMainPlayerIceBlock) && !al::isNerve(mMainPlayerIceBlock, &nrvFreezePlayerBlockDisappear)) {
            mMainPlayerIceBlock->end();
            trySetPostProcessingType(FreezePostProcessingType::PPDISABLED);
        }
    }

    GameModeBase::end();
}

void FreezeTagMode::pause() {
    GameModeBase::pause();

    mModeLayout->tryEnd();
}

void FreezeTagMode::unpause() {
    GameModeBase::unpause();

    mModeLayout->appear();
}

void FreezeTagMode::update() {
    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if (!player) {
        return;
    }

    // Update the mode timer
    mModeTimer->updateTimer();
    mModeTimer->disableControl();

    // Check for a decrease in the minute value (how survival time score as a runner is awarded)
    if ((mModeTimer->getTime().mMinutes < mInfo->mRoundTimer.mMinutes) && mInfo->mIsPlayerRunner) {
        mInfo->mPlayerTagScore.eventScoreSurvivalTime();
    }

    mInfo->mRoundTimer = mModeTimer->getTime();

    // Check if the time has run out for this round
    if (mModeTimer->isEnabled() && mModeTimer->getTimeCombined() <= 0.f) {
        endRound(false);
    }

    // Create list of runner and chaser player indicies
    // RCL TODO: only add people when the round starts, otherwise later joining runners will prevent a wipeout because they missed the round start
    //           removing people is fine, but does it always check if there are runners/chasers left? => check this, e.g. what happens if the last runner/chaser disconnects
    mInfo->mRunnerPlayers.clear();
    mInfo->mChaserPlayers.clear();

    for (int i = 0; i < mPuppetHolder->getSize(); i++) {
        PuppetInfo* other = Client::getPuppetInfo(i);

        if (!other || !other->isConnected || other->gameMode != mMode) {
            continue;
            // RCL TODO: add a third list for players in other game modes that is shown outside of a round?
        }

        if (other->isFreezeTagRunner) {
            mInfo->mRunnerPlayers.pushBack(other);
        } else {
            mInfo->mChaserPlayers.pushBack(other);
        }
    }

    // Verify you are never frozen on chaser team
    if (!mInfo->mIsPlayerRunner && mInfo->mIsPlayerFreeze) {
        trySetPlayerRunnerState(FreezeState::ALIVE);
    }

    mInvulnTime += Time::deltaTime;

    PuppetInfo* closestUnfrozenRunner = nullptr;

    if (mInfo->mIsRound && 3 <= mInvulnTime) {
        bool isPlayerAlive = !PlayerFunction::isPlayerDeadStatus(player);
        bool isPlayer2D    = ((PlayerActorHakoniwa*)player)->mDimKeeper->is2D;

        float closestDistance = 9999999.f;

        for (size_t i = 0; i < mPuppetHolder->getSize(); i++) {
            PuppetInfo* other = Client::getPuppetInfo(i);
            float distance = al::calcDistance(player, other->playerPos);

            if (!other->isConnected || !other->isInSameStage || other->gameMode != mMode) {
                continue;
            }

            // If this other player is the new closest, set the closest info to the current player
            if (distance < closestDistance && other->isFreezeTagRunner && !other->isFreezeTagFreeze) {
                closestDistance       = distance;
                closestUnfrozenRunner = other;
            }

            // skip if we're a chaser
            if (!mInfo->mIsPlayerRunner) {
                continue;
            }

            // Check if the chaser freezes us
            if (   !mInfo->mIsPlayerFreeze   // we're an unfrozen runner
                && isPlayerAlive             // that is alive
                && distance < 250.f          // and near
                && !other->isFreezeTagRunner // a chaser
                && other->is2D == isPlayer2D // that has the same dimension (2D/3D) as us
            ) {
                trySetPlayerRunnerState(FreezeState::FREEZE); // freeze ourselves
            }

            // Check if the runner unfreezes us
            float freezeMinTime = al::clamp(3.f + (mInfo->mFreezeCount * 0.5f), 3.f, 7.f); // cooldown of 3-7s, +0.5s per frozen runner
            if (   mInfo->mIsPlayerFreeze       // we're a frozen runner
                && freezeMinTime <= mInvulnTime // since some time (cooldown)
                && isPlayerAlive                // that is alive
                && distance < 200.f             // and near
                && other->isFreezeTagRunner     // another runner
                && !other->isFreezeTagFreeze    // that isn't frozen
                && other->is2D == isPlayer2D    // and has the same dimension (2D/3D) as us
            ) {
                trySetPlayerRunnerState(FreezeState::ALIVE); // unfreeze ourselves
            }
        }
    }

    // Set the target position to the closest puppet
    if (closestUnfrozenRunner) { // RCL TODO: only for chasers?
        mHintArrow->setTarget(&closestUnfrozenRunner->playerPos);
    } else {
        mHintArrow->setTarget(nullptr);
    }

    // Update recovery event timer
    if (0 < mRecoveryEventFrames) {
        mRecoveryEventFrames--;
        if (mRecoveryEventFrames <= 0) {
            tryEndRecoveryEvent();
        }
    }

    // Update endgame event (show wipeout for 6 seconds)
    if (mIsEndgameActive) {
        mEndgameTimer += Time::deltaTime;
        if (mEndgameTimer > 6.f) {
            mInfo->mIsPlayerRunner = true;
            mInvulnTime = 0.f;
            sendFreezePacket(FreezeUpdateType::PLAYER);

            mIsEndgameActive = false;
            tryStartRecoveryEvent(true);
        }
    }

    // If our score changes, tell that the other players
    FreezeTagScore* score = &mInfo->mPlayerTagScore;
    if (score->mScore != score->mPrevScore) {
        score->mPrevScore = score->mScore;
        sendFreezePacket(FreezeUpdateType::PLAYER);
    };

    // Main player's ice block state and post processing
    if (mInfo->mIsPlayerFreeze) {
        if (!al::isAlive(mMainPlayerIceBlock)) {
            mMainPlayerIceBlock->appear();
            trySetPostProcessingType(FreezePostProcessingType::PPFROZEN);
        }

        // Lock block onto player
        al::setTrans(mMainPlayerIceBlock, al::getTrans(player));
        al::setQuat(mMainPlayerIceBlock, al::getQuat(player));
    } else {
        if (al::isAlive(mMainPlayerIceBlock) && mMainPlayerIceBlock->mIsLocked) {
            mMainPlayerIceBlock->end();
            trySetPostProcessingType(FreezePostProcessingType::PPDISABLED);
        }
    }

    // Up => Toggle Role (chaser/runner)
    if (   !mInfo->mIsRound          // not during a round
        && !mInfo->mIsPlayerFreeze   // not when frozen
        && mRecoveryEventFrames == 0 // not in recovery
        && !mIsEndgameActive         // not in endgame (wipeout)
        && al::isPadTriggerUp(-1)    // D-Pad Up
        && !al::isPadHoldZR(-1)      // not ZR
        && !al::isPadHoldL(-1)       // not L
        && !al::isPadHoldR(-1)       // not R
    ) {
        mInfo->mIsPlayerRunner = !mInfo->mIsPlayerRunner;
        mInvulnTime = 0.f;

        sendFreezePacket(FreezeUpdateType::PLAYER);
    }

    // L + Down => Reset Score
    if (   !mInfo->mIsPlayerFreeze   // not when frozen
        && mRecoveryEventFrames == 0 // not in recovery
        && !mIsEndgameActive         // not in endgame (wipeout)
        && al::isPadHoldL(-1)        // hold L
        && al::isPadTriggerDown(-1)  // D-Pad Down
    ) {
        mInfo->mPlayerTagScore.resetScore();
    }

    // [Host] R + Up => Start Round
    if (   mInfo->mIsHostMode     // when host
        && !mInfo->mIsRound       // not during a round
        && al::isPadHoldR(-1)     // hold R
        && al::isPadTriggerUp(-1) // D-Pad Up
    ) {
        startRound(mInfo->mRoundLength);
        sendFreezePacket(FreezeUpdateType::ROUNDSTART);
    }

    // [Host] R + Down => End Round
    if (   mInfo->mIsHostMode       // when host
        && mInfo->mIsRound          // only during a round
        && al::isPadHoldR(-1)       // hold R
        && al::isPadTriggerDown(-1) // D-Pad Down
    ) {
        endRound(true);
        sendFreezePacket(FreezeUpdateType::ROUNDCANCEL);
    }

    // Debug freeze buttons
    if (mInfo->mIsDebugMode) {
        if (mInfo->mIsPlayerRunner) {
            // [Debug] X + Right => Unfreeze
            if (al::isPadHoldX(-1) && al::isPadTriggerRight(-1)) {
                trySetPlayerRunnerState(FreezeState::ALIVE);
            }
            // [Debug] Y + Right => Freeze
            if (al::isPadHoldY(-1) && al::isPadTriggerRight(-1)) {
                trySetPlayerRunnerState(FreezeState::FREEZE);
            }
        }
        // [Debug] A + Right => Score += 1
        if (al::isPadHoldA(-1) && al::isPadTriggerRight(-1)) {
            mInfo->mPlayerTagScore.eventScoreDebug();
        }
        // [Debug] A + Left => Set time to 01:05
        if (al::isPadHoldA(-1) && al::isPadTriggerLeft(-1)) {
            mModeTimer->setTime(0.f, 5, 1, 0);
        }
        // [Debug] B + Right => Wipeout
        if (al::isPadHoldB(-1) && al::isPadTriggerRight(-1)) {
            tryStartEndgameEvent();
        }
    }

    // Verify that the standard HUD is hidden (coins)
    if (!mCurScene->mSceneLayout->isEnd()) {
        mCurScene->mSceneLayout->end();
    }

    // Spectator camera
    if (!mTicket->mIsActive && mInfo->mIsPlayerFreeze) { // enable the spectator camera when frozen
        al::startCamera(mCurScene, mTicket, -1);
        al::requestStopCameraVerticalAbsorb(mCurScene);
    } else if (mTicket->mIsActive && !mInfo->mIsPlayerFreeze) { // disable the spectator camera when unfrozen
        al::endCamera(mCurScene, mTicket, 0, false);
        al::requestStopCameraVerticalAbsorb(mCurScene);
    } else if (mTicket->mIsActive && mInfo->mIsPlayerFreeze) { // update spectator camera
        updateSpectateCam(player);
    }
}

bool FreezeTagMode::showNameTag(PuppetInfo* other) {
    // show name tags for non-players and our team mates
    return other->gameMode != mMode
        || ( isPlayerRunner() &&  other->isFreezeTagRunner)
        || (!isPlayerRunner() && !other->isFreezeTagRunner)
    ;
}

bool FreezeTagMode::showNameTagEverywhere(PuppetActor* actor) {
    // show the name tags of frozen players everywhere (regardless of distance)
    PuppetInfo* other = actor->getInfo();
    return other->gameMode == mMode
        && other->isFreezeTagRunner
        && other->isFreezeTagFreeze
    ;
}

void FreezeTagMode::debugMenuControls(sead::TextWriter* gTextWriter) {
    gTextWriter->printf("- L + ← | Enable/disable Freeze Tag [FT]\n");
    gTextWriter->printf("- [FT] ↑ | Switch between runners and chasers\n");
    gTextWriter->printf("- [FT] L + ↓ | Reset score\n");

    if (mInfo->mIsHostMode) {
        gTextWriter->printf("- [FT][Host] R + ↑ | Start round\n");
        gTextWriter->printf("- [FT][Host] R + ↓ | End round\n");
    }

    if (mInfo->mIsDebugMode) {
        gTextWriter->printf("- [FT][Debug] A + → | Increment score\n");
        gTextWriter->printf("- [FT][Debug] A + ← | Set time to 01:05\n");
        gTextWriter->printf("- [FT][Debug] B + → | Wipeout\n");
        if (mInfo->mIsPlayerRunner) {
            gTextWriter->printf("- [FT][Debug][Runner] X + → | Unfreeze\n");
            gTextWriter->printf("- [FT][Debug][Runner] Y + → | Freeze\n");
        }
    }

    if (mTicket && mTicket->mIsActive && mInfo->mIsPlayerFreeze) {
        gTextWriter->printf("- [FT][Frozen] ← | Spectate previous player\n");
        gTextWriter->printf("- [FT][Frozen] → | Spectate next player\n");
    }
}

void FreezeTagMode::debugMenuPlayer(sead::TextWriter* gTextWriter, PuppetInfo* other) {
    if (other && other->gameMode != mMode) {
        gTextWriter->printf("Freeze-Tag: N/A\n");
        return;
    }

    bool isRunner = other ? other->isFreezeTagRunner : mInfo->mIsPlayerRunner;
    bool isFrozen = other ? other->isFreezeTagFreeze : mInfo->mIsPlayerFreeze;

    gTextWriter->printf(
        "Freeze-Tag: %s%s\n",
        isRunner ? "Runner" : "Chaser",
        isFrozen ? " (Frozen)" : ""
    );
}

void FreezeTagMode::sendFreezePacket(FreezeUpdateType updateType) {
    mNextUpdateType = updateType;
    Client::sendGameModeInfPacket();
    mNextUpdateType = FreezeUpdateType::PLAYER;
}

void FreezeTagMode::onHakoniwaSequenceFirstStep(HakoniwaSequence* sequence) {
    mWipeHolder = sequence->mWipeHolder;
}
