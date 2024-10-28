#include "server/coinrunners/CoinRunnerMode.hpp"

#include "sead/heap/seadHeapMgr.h"

#include "actors/PuppetActor.h"

#include "al/async/FunctorV0M.hpp"
#include "al/util.hpp"

#include "game/Player/PlayerFunction.h"
#include "game/Player/PlayerHitPointData.h"
#include "game/StageScene/StageScene.h"

#include "logger.hpp"

#include "server/Client.hpp"
#include "server/DeltaTime.hpp"

#include "server/gamemode/GameMode.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/gamemode/GameModeFactory.hpp"

#include "server/coinrunners/CoinRunnerScore.hpp"
#include "server/coinrunners/CoinHintArrow.h"
#include "server/coinrunners/CoinPlayerBlock.h"
#include "server/coinrunners/CoinRunnerIcon.h"

CoinRunnerMode::CoinRunnerMode(const char* name) : GameModeBase(name) {}

void CoinRunnerMode::init(const GameModeInitInfo& info) {
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
        mInfo = (CoinRunnerInfo*)curGameInfo;
        mModeTimer = new GameModeTimer(mInfo->mRoundTimer);
        Logger::log("Reinitialized timer with time %d:%.2d\n", mInfo->mRoundTimer.mMinutes, mInfo->mRoundTimer.mSeconds);
    } else {
        if (curGameInfo) {
            delete curGameInfo; // attempt to destory previous info before creating new one
        }

        mInfo = GameModeManager::instance()->createModeInfo<CoinRunnerInfo>();

        mModeTimer = new GameModeTimer();
    }

    sead::ScopedCurrentHeapSetter heapSetterr(GameModeManager::getSceneHeap());

    mModeLayout = new CoinRunnerIcon("CoinRunnerIcon", *info.mLayoutInitInfo);
    mInfo->mPlayerTagScore.setTargetLayout(mModeLayout);
    mInfo->mPlayerTagScore.initBuffer();

    mInfo->mRunnerPlayers.allocBuffer(0x10, al::getSceneHeap());
    mInfo->mChaserPlayers.allocBuffer(0x10, al::getSceneHeap());
    mInfo->mOtherPlayers.allocBuffer(0x10,  al::getSceneHeap());

    // Create main player's ice block
    mMainPlayerIceBlock = new CoinPlayerBlock("MainPlayerBlock");
    mMainPlayerIceBlock->init(*info.mActorInitInfo);

    // Create hint arrow
    mHintArrow = new CoinHintArrow("ChaserHintArrow");
    mHintArrow->init(*info.mActorInitInfo);
}

void CoinRunnerMode::processPacket(Packet* _packet) {
    CoinRunnerPacket* packet     = (CoinRunnerPacket*)_packet;
    CoinUpdateType updateType = packet->updateType();

    /**
     * Ignore legacy game mode packets for other game modes
     *
     * Legacy Coin-Tag packets that we are interested in should have been automatically
     * transformed from LEGACY to COINRUNNER by the logic in the gameMode() function.
     */
    if (packet->gameMode() == GameMode::LEGACY) {
        return;
    }

    PuppetInfo* other = Client::findPuppetInfo(packet->mUserID, false);
    if (!other) {
        return;
    }

    if (updateType == CoinUpdateType::PLAYER) {
        tryScoreEvent(packet, other);

        // When puppet transitioning from frozen to unfrozen, disable the fall off flag
        if (other->ftIsFrozen() && !packet->isCoin) {
            other->isCoinRunnerFallenOff = false;
        }

        other->isCoinRunnerRunner = packet->isRunner;
        other->isCoinRunnerFreeze = packet->isCoin;
        other->freezeTagScore    = packet->score;
    }

    if (isRound()) {
        // Abort round early on receiving cancel packet
        if (updateType == CoinUpdateType::ROUNDCANCEL) {
            if (packet->mPacketSize < sizeof(CoinRunnerRoundCancelPacket) - sizeof(Packet)) { // From a legacy client
                endRound(true);
                mInfo->mPlayerTagScore.eventRoundCancelled(other->puppetName);
            } else { // From a new client
                CoinRunnerRoundCancelPacket* roundCancel = (CoinRunnerRoundCancelPacket*)packet;
                if (!roundCancel->onlyForLegacy) {
                    endRound(true);
                    mInfo->mPlayerTagScore.eventRoundCancelled(other->puppetName);
                }
            }
        }

        if (updateType == CoinUpdateType::FALLOFF) {
            other->isCoinRunnerFallenOff = true;

            if (isPlayerChaser()) {
                mInfo->mPlayerTagScore.eventScoreFallOff();
            }
        }
    } else if (updateType == CoinUpdateType::ROUNDSTART) {
        CoinRunnerRoundStartPacket* roundStart = (CoinRunnerRoundStartPacket*)packet;
        startRound(al::clamp(roundStart->roundTime, u8(2), u8(60))); // Start round if round not already started
        mInfo->mPlayerTagScore.eventRoundStarted(other->puppetName);
    }
}

Packet* CoinRunnerMode::createPacket() {
    if (!isModeActive()) {
        DisabledGameModeInf* packet = new DisabledGameModeInf(Client::getClientId());
        packet->setUpdateType(0); // so that legacy coinrunners clients don't wrongly interpret this as a round start
        return packet;
    }

    if (mNextUpdateType == CoinUpdateType::ROUNDSTART) {
        CoinRunnerRoundStartPacket* packet = new CoinRunnerRoundStartPacket();
        packet->mUserID   = Client::getClientId();
        packet->roundTime = u8(mInfo->mRoundLength);
        return packet;
    }

    if (mNextUpdateType == CoinUpdateType::ROUNDCANCEL) {
        CoinRunnerRoundCancelPacket* packet = new CoinRunnerRoundCancelPacket();
        packet->mUserID       = Client::getClientId();
        packet->onlyForLegacy = mCancelOnlyLegacy;
        mCancelOnlyLegacy     = false;
        return packet;
    }

    CoinRunnerPacket* packet = new CoinRunnerPacket();
    packet->mUserID  = Client::getClientId();
    packet->isRunner = isPlayerRunner();
    packet->isCoin = isPlayerCoin();
    packet->score    = getScore();
    packet->setUpdateType(mNextUpdateType);

    return packet;
}

void CoinRunnerMode::begin() {
    unpause();

    mInvulnTime         = 0.f;
    mSpectateIndex      = -1; // ourself
    mPrevSpectateIndex  = -2;
    mIsScoreEventsValid = true;

    if (isRound()) {
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

void CoinRunnerMode::end() {
    pause();

    mInvulnTime         = 0.f;
    mIsScoreEventsValid = false;

    mCurScene->mSceneLayout->start();

    if (!GameModeManager::instance()->isPaused()) {
        if (isPlayerCoin()) {
            trySetPlayerRunnerState(CoinState::ALIVECoin);
        }

        if (mTicket->mIsActive) {
            al::endCamera(mCurScene, mTicket, 0, false);
        }

        if (al::isAlive(mMainPlayerIceBlock) && !al::isNerve(mMainPlayerIceBlock, &nrvCoinPlayerBlockDisappear)) {
            mMainPlayerIceBlock->end();
            trySetPostProcessingType(CoinPostProcessingType::PPDISABLED);
        }

        if (isRound()) {
            endRound(true);
        }
    }

    GameModeBase::end();
}

void CoinRunnerMode::pause() {
    GameModeBase::pause();

    mModeLayout->tryEnd();
}

void CoinRunnerMode::unpause() {
    GameModeBase::unpause();

    mModeLayout->appear();
}

void CoinRunnerMode::update() {
    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if (!player) {
        return;
    }

    // Update the mode timer
    mModeTimer->updateTimer();
    mModeTimer->disableControl();

    // Check for a decrease in the minute value (how survival time score as a runner is awarded)
    if (isPlayerRunner() && mModeTimer->getTime().mMinutes < mInfo->mRoundTimer.mMinutes) {
        mInfo->mPlayerTagScore.eventScoreSurvivalTime();
    }

    mInfo->mRoundTimer = mModeTimer->getTime();

    // Check if the time has run out for this round
    if (mModeTimer->isEnabled() && mModeTimer->getTimeCombined() <= 0.f) {
        endRound(false);
    }

    // keep track of connected runner and chaser counts
    int enoughRunners = Client::isSocketActive() ? isPlayerRunner() : 0;
    int enoughChasers = Client::isSocketActive() ? isPlayerChaser() : 0;

    // Create list of runner and chaser player indicies
    mInfo->mRunnerPlayers.clear();
    mInfo->mChaserPlayers.clear();
    mInfo->mOtherPlayers.clear();
    for (int i = 0; i < mPuppetHolder->getSize(); i++) {
        PuppetInfo* other = Client::getPuppetInfo(i);

        if (!other) {
            continue;
        }

        if (!other->isConnected && !other->isFreezeInRound) {
            continue;
        }

        if (other->gameMode != mMode) {
            mInfo->mOtherPlayers.pushBack(other);
            other->isFreezeInRound = false;
            continue;
        }

        if (isRound() && !other->isFreezeInRound) {
            mInfo->mOtherPlayers.pushBack(other);
            continue;
        }

        if (other->ftIsRunner()) {
            enoughRunners += other->isConnected;
            mInfo->mRunnerPlayers.pushBack(other);
        } else {
            enoughChasers += other->isConnected;
            mInfo->mChaserPlayers.pushBack(other);
        }
    }

    // Verify you are never frozen on chaser team
    if (isPlayerChaser() && isPlayerCoin()) {
        trySetPlayerRunnerState(CoinState::ALIVECoin);
    }

    mInvulnTime += Time::deltaTime;

    PuppetInfo* closestUnfrozenRunner = nullptr;

    if (isRound() && 3 <= mInvulnTime) {
        bool isPlayerAlive = !PlayerFunction::isPlayerDeadStatus(player);
        bool isPlayer2D    = ((PlayerActorHakoniwa*)player)->mDimKeeper->is2D;

        sead::Vector3f playerPos = al::getTrans(player);
        float closestDistanceSq = 9999999.f;
        float freezeMinTime = isPlayerRunner() ? al::clamp(3.f + (mInfo->mCoinCount * 0.5f), 3.f, 7.f) : 0.f; // cooldown of 3-7s, +0.5s per frozen runner

        for (size_t i = 0; i < mPuppetHolder->getSize(); i++) {
            PuppetInfo* other = Client::getPuppetInfo(i);

            if (!other->isConnected || !other->isInSameStage || other->gameMode != mMode) {
                continue;
            }

            if (isPlayerRunner()) { // if we're a runner
                // Check if the chaser freezes us
                bool chaserCoinsUs = (
                       isPlayerUncoin()        // we're an unfrozen runner
                    && isPlayerAlive             // that is alive
                    && other->ftIsChaser()       // a chaser
                    && other->is2D == isPlayer2D // that has the same dimension (2D/3D) as us
                );

                // Check if the runner unfreezes us
                bool runnerUnfreezesUs = (
                       !chaserCoinsUs
                    && isPlayerCoin()             // we're a frozen runner
                    && freezeMinTime <= mInvulnTime // since some time (cooldown)
                    && isPlayerAlive                // that is alive
                    && other->ftIsRunner()          // another runner
                    && other->ftIsUnfrozen()        // that isn't frozen
                    && other->is2D == isPlayer2D    // and has the same dimension (2D/3D) as us);
                );

                if (chaserCoinsUs || runnerUnfreezesUs) {
                    float distanceSq = vecDistanceSq(playerPos, other->playerPos);

                    if (chaserCoinsUs && distanceSq < 62500.f) { // non-squared: 250.0
                        trySetPlayerRunnerState(CoinState::Coin); // freeze ourselves
                    } else if (runnerUnfreezesUs && distanceSq < 40000.f) { // non-squared: 200.0
                        trySetPlayerRunnerState(CoinState::ALIVECoin); // unfreeze ourselves
                    }

                    break;
                }
            } else if (other->ftIsRunner() && other->ftIsUnfrozen()) { // if we're a chaser, and the other player is a unfrozen runner
                float distanceSq = vecDistanceSq(playerPos, other->playerPos);
                // If this other player is the new closest, set the closest info to the current player
                if (!closestUnfrozenRunner || distanceSq < closestDistanceSq) {
                    closestDistanceSq     = distanceSq;
                    closestUnfrozenRunner = other;
                }
            }
        }
    }

    // Set the arrow target position to the closest unfrozen runner
    if (closestUnfrozenRunner) {
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
    if (isWipeout()) {
        mEndgameTimer += Time::deltaTime;
        if (mEndgameTimer > 6.f) {
            mInfo->mIsPlayerRunner = true;
            mInvulnTime = 0.f;
            sendCoinPacket(CoinUpdateType::PLAYER);

            mIsEndgameActive = false;
            tryStartRecoveryEvent(true);
        }
    }

    // If our score changes, tell that the other players
    CoinRunnerScore* score = &mInfo->mPlayerTagScore;
    if (score->mScore != score->mPrevScore) {
        score->mPrevScore = score->mScore;
        sendCoinPacket(CoinUpdateType::PLAYER);
    };

    // Main player's ice block state and post processing
    if (isPlayerCoin()) {
        if (!al::isAlive(mMainPlayerIceBlock)) {
            mMainPlayerIceBlock->appear();
            trySetPostProcessingType(CoinPostProcessingType::PPFROZEN);
        }

        // Lock block onto player
        al::setTrans(mMainPlayerIceBlock, al::getTrans(player));
        al::setQuat(mMainPlayerIceBlock, al::getQuat(player));
    } else {
        if (al::isAlive(mMainPlayerIceBlock) && mMainPlayerIceBlock->mIsLocked) {
            mMainPlayerIceBlock->end();
            trySetPostProcessingType(CoinPostProcessingType::PPDISABLED);
        }
    }

    // Not enough chasers/runners to continuing this round => cancel it
    if (isRound() && (!enoughChasers || !enoughRunners)) {
        mDisconnectTimer += Time::deltaTime;
        /**
         * 5 seconds grace period:
         * - to allow fixing it w/ automatic reconnects
         * - to not wrongly trigger by chasers changing to runners at round end (by slightly misalligned timers)
         */
        if (5.0f < mDisconnectTimer) {
            mDisconnectTimer = 0.0f;

            endRound(true);

            mCancelOnlyLegacy = true;
            sendCoinPacket(CoinUpdateType::ROUNDCANCEL);

            if (!enoughChasers) {
                mInfo->mPlayerTagScore.eventNotEnoughChasersToContinue();
            } else {
                mInfo->mPlayerTagScore.eventNotEnoughRunnersToContinue();
            }
        }
    } else {
        mDisconnectTimer = 0.0f;
    }

    // Up => Toggle Role (chaser/runner)
    if (   !isRound()                // not during a round
        && isPlayerUncoin()        // not when frozen
        && mRecoveryEventFrames == 0 // not in recovery
        && !isWipeout()              // not in endgame (wipeout)
        && al::isPadTriggerUp(-1)    // D-Pad Up
        && !al::isPadHoldZR(-1)      // not ZR
        && !al::isPadHoldL(-1)       // not L
        && !al::isPadHoldR(-1)       // not R
    ) {
        mInfo->mIsPlayerRunner = !mInfo->mIsPlayerRunner;
        mInvulnTime = 0.f;

        sendCoinPacket(CoinUpdateType::PLAYER);
    }

    // L + Down => Reset Score
    if (   isPlayerUncoin()        // not when frozen
        && mRecoveryEventFrames == 0 // not in recovery
        && !isWipeout()              // not in endgame (wipeout)
        && al::isPadHoldL(-1)        // hold L
        && al::isPadTriggerDown(-1)  // D-Pad Down
    ) {
        mInfo->mPlayerTagScore.resetScore();
    }

    // [Host] R + Up => Start Round
    if (   isHost()               // when host
        && !isRound()             // not during a round
        && al::isPadHoldR(-1)     // hold R
        && al::isPadTriggerUp(-1) // D-Pad Up
    ) {
        if (enoughRunners < 1) {
            mInfo->mPlayerTagScore.eventNotEnoughRunnersToStart();
        } else if (enoughChasers < 1) {
            mInfo->mPlayerTagScore.eventNotEnoughChasersToStart();
        } else {
            startRound(mInfo->mRoundLength);
            sendCoinPacket(CoinUpdateType::ROUNDSTART);
        }
    }

    // [Host] R + Down => End Round
    if (   isHost()                 // when host
        && isRound()                // only during a round
        && al::isPadHoldR(-1)       // hold R
        && al::isPadTriggerDown(-1) // D-Pad Down
    ) {
        endRound(true);
        sendCoinPacket(CoinUpdateType::ROUNDCANCEL);
    }

    // Debug freeze buttons
    if (mInfo->mIsDebugMode) {
        if (isPlayerRunner()) {
            // [Debug] X + Right => Unfreeze
            if (al::isPadHoldX(-1) && al::isPadTriggerRight(-1)) {
                trySetPlayerRunnerState(CoinState::ALIVECoin);
            }
            // [Debug] Y + Right => Coin
            if (al::isPadHoldY(-1) && al::isPadTriggerRight(-1)) {
                trySetPlayerRunnerState(CoinState::Coin);
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
    if (!mTicket->mIsActive && isPlayerCoin()) { // enable the spectator camera when frozen
        al::startCamera(mCurScene, mTicket, -1);
        al::requestStopCameraVerticalAbsorb(mCurScene);
    } else if (mTicket->mIsActive && isPlayerUncoin()) { // disable the spectator camera when unfrozen
        al::endCamera(mCurScene, mTicket, 0, false);
        al::requestStopCameraVerticalAbsorb(mCurScene);
    } else if (mTicket->mIsActive && isPlayerCoin()) { // update spectator camera
        updateSpectateCam(player);
    }
}

bool CoinRunnerMode::showNameTag(PuppetInfo* other) {
    // show name tags for non-players and our team mates
    return other->gameMode != mMode
        || (isPlayerRunner() && other->ftIsRunner())
        || (isPlayerChaser() && other->ftIsChaser())
    ;
}

bool CoinRunnerMode::showNameTagEverywhere(PuppetActor* actor) {
    // show the name tags of frozen players everywhere (regardless of distance)
    PuppetInfo* other = actor->getInfo();
    return other->gameMode == mMode
        && other->ftIsRunner()
        && other->ftIsFrozen()
    ;
}

void CoinRunnerMode::debugMenuControls(sead::TextWriter* gTextWriter) {
    gTextWriter->printf("- L + ← | Enable/disable Coin Tag [FT]\n");
    gTextWriter->printf("- [FT] ↑ | Switch between runners and chasers\n");
    gTextWriter->printf("- [FT] L + ↓ | Reset score\n");

    if (isHost()) {
        gTextWriter->printf("- [FT][Host] R + ↑ | Start round\n");
        gTextWriter->printf("- [FT][Host] R + ↓ | End round\n");
    }

    if (mInfo->mIsDebugMode) {
        gTextWriter->printf("- [FT][Debug] A + → | Increment score\n");
        gTextWriter->printf("- [FT][Debug] A + ← | Set time to 01:05\n");
        gTextWriter->printf("- [FT][Debug] B + → | Wipeout\n");
        if (isPlayerRunner()) {
            gTextWriter->printf("- [FT][Debug][Runner] X + → | Unfreeze\n");
            gTextWriter->printf("- [FT][Debug][Runner] Y + → | Coin\n");
        }
    }

    if (mTicket && mTicket->mIsActive && isPlayerCoin()) {
        gTextWriter->printf("- [FT][Frozen] ← | Spectate previous player\n");
        gTextWriter->printf("- [FT][Frozen] → | Spectate next player\n");
    }
}

void CoinRunnerMode::debugMenuPlayer(sead::TextWriter* gTextWriter, PuppetInfo* other) {
    if (other && other->gameMode != mMode) {
        gTextWriter->printf("Coin-Tag: N/A\n");
        return;
    }

    bool isRunner = other ? other->ftIsRunner() : isPlayerRunner();
    bool isFrozen = other ? other->ftIsFrozen() : isPlayerCoin();

    gTextWriter->printf(
        "Coin-Tag: %s%s\n",
        isRunner ? "Runner" : "Chaser",
        isFrozen ? " (Frozen)" : ""
    );
}

void CoinRunnerMode::sendCoinPacket(CoinUpdateType updateType) {
    mNextUpdateType = updateType;
    Client::sendGameModeInfPacket();
    mNextUpdateType = CoinUpdateType::PLAYER;
}

void CoinRunnerMode::onHakoniwaSequenceFirstStep(HakoniwaSequence* sequence) {
    mWipeHolder = sequence->mWipeHolder;
}
