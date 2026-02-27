#include "al/Library/Camera/CameraDirector.h"
#include "al/Library/Camera/CameraPoseUpdater.h"
#include "al/Library/Camera/CameraTicket.h"
#include "al/Library/Camera/CameraUtil.h"
#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/Play/Layout/WipeHolder.h"
#include "al/Library/Scene/SceneUtil.h"

#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Player/PlayerAnimator.h"
#include "game/Player/PlayerHackKeeper.h"
#include "game/Player/PlayerRecoverySafetyPoint.h"
#include "game/Util/ObjUtil.h"

#include "../src/smallMarioHooks.hpp"
#include "cameras/CameraPoserActorSpectate.h"
#include "helpers.hpp"
#include "logger.hpp"
#include "puppets/PuppetInfo.h"
#include "server/shine-thief/ShineThiefMode.hpp"
#include "TwistsConfig.hpp"

void ShineThiefMode::updateIntroCamera() {
    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if (!player)
        return;

    // Get the camera director and verify spectate camera is active
    al::CameraPoser* curPoser = nullptr;
    al::CameraDirector* director = mCurScene->getCameraDirector();

    if (director) {
        al::CameraPoseUpdater* updater = director->getPoseUpdater(0);
        if (updater && updater->mTicket) {
            curPoser = updater->mTicket->mPoser;
        }
    }

    // Verify this is the spectate camera poser
    if (curPoser && al::isEqualString(curPoser->getName(), "CameraPoserActorSpectate")) {
        cc::CameraPoserActorSpectate* spectatePoser = (cc::CameraPoserActorSpectate*)curPoser;
        spectatePoser->setPlayer(player);

        // Point camera at the shine position
        spectatePoser->setTargetActor(&mInfo->shinePos);
    }
}

void ShineThiefMode::startRound(int roundMinutes) {
    // Set round state
    mInfo->mIsRound = true;
    mModeTimer->enableTimer();
    mModeTimer->disableControl();
    mModeTimer->setTimerDirection(false);
    mModeTimer->setTime(0.f, 59, roundMinutes - 1, 0);
    mMainPlayerCaptureBlock->appear();

    // Setup intro animation
    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if (!player || !mWipeHolder) {
        Logger::log("Warning: Could not start round intro - player or wipe holder unavailable\n");
        return;
    }

    if (mHostStartPos == sead::Vector3f::zero) {
        // If not set by packet, use current player position (host case)
        mHostStartPos = al::getTrans(player);
        Logger::log("Round starting - Host pos: %.1f, %.1f, %.1f\n", mHostStartPos.x, mHostStartPos.y, mHostStartPos.z);
    }

    // Start intro spectate camera pointing at shine
    if (mSpectateTicket) {
        al::startCamera(mCurScene, mSpectateTicket, -1);
        al::requestStopCameraVerticalAbsorb(mCurScene);
    }

    // Start fade to black (15 frames)
    int fadeDuration = mRecoveryEventLength / 4;
    mWipeHolder->startClose("FadeBlack", fadeDuration);

    // Start intro sequence (4.5 seconds at 60fps)
    mRoundIntroFrames = 270;
    mIsRoundIntro = true;

    Logger::log("Round intro started: %d frames, fade: %d frames\n", mRoundIntroFrames, fadeDuration);
}

void ShineThiefMode::updateRoundIntro() {
    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if (!player)
        return;

    const int fadeDuration = mRecoveryEventLength / 4;      // 15 frames
    const int fadeCompleteFrame = 270 - fadeDuration;       // 255
    const int teleportFrame = fadeCompleteFrame - 30;       // 225
    const int countdown2Frame = teleportFrame - 60;         // 165
    const int countdown1Frame = teleportFrame - 120;        // 105
    const int countdownGoFrame = teleportFrame - 180;       // 45
    const int releaseControlFrame = countdownGoFrame - 20;  // 25
    const int stopSpectateFrame = 60;

    // First frame - teleport and setup
    if (mRoundIntroFrames == 270) {
        mMainPlayerCaptureBlock->appear();
        al::setTrans(mMainPlayerCaptureBlock, getShinePos());

        updateIntroCamera();
    }

    // Update camera continuously until stop frame
    if (mRoundIntroFrames > stopSpectateFrame) {
        updateIntroCamera();
    }

    // Stop spectate camera
    if (mRoundIntroFrames == stopSpectateFrame) {
        if (mSpectateTicket && mSpectateTicket->mIsActiveCamera) {
            al::endCamera(mCurScene, mSpectateTicket, 0, false);
            al::requestStopCameraVerticalAbsorb(mCurScene);
        }
    }

    // Freeze player and start countdown - TELEPORT ONLY ONCE HERE
    if (mRoundIntroFrames == teleportFrame) {
        if (player->getPlayerHackKeeper()->mHackActor)
            player->getPlayerHackKeeper()->cancelHackArea();

        player->startDemoPuppetable();
        rs::faceToCamera(player);
        player->mAnimator->endSubAnim();
        player->mAnimator->startAnim("RaceResultWin");

        if (mWipeHolder)
            mWipeHolder->startOpen(fadeDuration);
        mModeLayout->showCountdown(3);

        // TELEPORT PLAYER ONCE - moved from continuous loop below
        if (mHostStartPos != sead::Vector3f::zero) {
            Logger::log("Teleporting player to host pos: %.1f, %.1f, %.1f\n", mHostStartPos.x, mHostStartPos.y, mHostStartPos.z);
            al::setTrans(player, mHostStartPos);
        } else {
            Logger::log("WARNING: Host start pos is zero!\n");
        }
    }

    // Countdown
    if (mRoundIntroFrames == countdown2Frame)
        mModeLayout->showCountdown(2);
    else if (mRoundIntroFrames == countdown1Frame)
        mModeLayout->showCountdown(1);
    else if (mRoundIntroFrames == countdownGoFrame)
        mModeLayout->showCountdown(0);
    else if (mRoundIntroFrames == releaseControlFrame) {
        player->endDemoPuppetable();
        mModeLayout->startCountdownScaleDown();
    }

    mRoundIntroFrames--;

    // Complete intro
    if (mRoundIntroFrames <= 0) {
        mIsRoundIntro = false;
        // Reset host start pos for next round
        mHostStartPos = sead::Vector3f::zero;
    }
}

void ShineThiefMode::endRound(bool isAbort) {
    mInfo->mIsRound = false;
    mModeTimer->disableTimer();
    mMainPlayerCaptureBlock->end();

    // Clean up intro state if still active
    if (mIsRoundIntro) {
        mIsRoundIntro = false;
        mRoundIntroFrames = 0;
        mModeLayout->hideCountdown();

        if (mSpectateTicket && mSpectateTicket->mIsActiveCamera) {
            al::endCamera(mCurScene, mSpectateTicket, 0, false);
        }

        PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
        if (player)
            player->endDemoPuppetable();
    }

    // Reset host position
    mHostStartPos = sead::Vector3f::zero;

    if (!mIsEndgameActive) {
        if (mInfo->mIsPlayerHolder && !isAbort)
            mInfo->mPlayerTagScore.eventScoreRunnerWin();
        trySetPlayerHolderState(false);
    }
}

bool ShineThiefMode::trySetPlayerHolderState(bool hasShine) {
    if (!getPlayerActorHakoniwa() || mInfo->mIsPlayerHolder == hasShine)
        return false;

    mInvulnTime = 0.f;
    mInfo->mIsPlayerHolder = hasShine;
    sendShineThiefPacket(ShineThiefUpdateType::PLAYER);

    sead::Vector3f sPos = al::getTrans(getPlayerActorHakoniwa());
    sPos.y += TwistsConfig::isSmallMarioEnabled() ? 100.f * ::scale : 100.f;
    setShinePos(sPos);

    return true;
}

bool ShineThiefMode::tryStealShine() {
    if (mInfo->mIsPlayerHolder || !mInfo->mIsRound)
        return false;

    mInfo->mIsPlayerHolder = true;
    mInfo->mPlayerTagScore.eventScoreCapture();
    mInvulnTime = 0.f;

    mShineHoldTimer = 0.f;  // Reset timer when stealing shine

    sendShineThiefPacket(ShineThiefUpdateType::PLAYER);

    return true;
}

void ShineThiefMode::forceDropShine() {
    if (!mInfo->mIsPlayerHolder)
        return;

    mInfo->mIsPlayerHolder = false;
    mInvulnTime = 0.f;

    sendShineThiefPacket(ShineThiefUpdateType::PLAYER);
}

void ShineThiefMode::tryScoreEvent(ShineThiefInf* incomingPacket, PuppetInfo* sourcePuppet) {
    if (!mCurScene || !sourcePuppet || !mCurScene->mIsAlive)
        return;

    PlayerActorBase* playerBase = (PlayerActorBase*)rs::getPlayerActor(mCurScene);
    if (!playerBase)
        return;

    float puppetDistance = al::calcDistance(playerBase, sourcePuppet->playerPos);
    if (puppetDistance >= 600.f)
        return;
}

bool ShineThiefMode::tryStartRecoveryEvent(bool isEndgame) {
    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if (!player || mRecoveryEventFrames > 0 || !mWipeHolder)
        return false;

    int multiplier = isEndgame ? 2 : 1;
    mRecoveryEventFrames = (mRecoveryEventLength / 2) * multiplier;
    mWipeHolder->startClose("FadeBlack", (mRecoveryEventLength / 4) * multiplier);

    if (!isEndgame) {
        mRecoverySafetyPoint = player->mRecoverySafetyPoint->getSafetyPoint();

        if (mInfo->mIsPlayerHolder && mInfo->mIsRound) {
            // Drop the shine
            trySetPlayerHolderState(false);

            // Place shine randomly
            placeShine();

            Logger::log("Player fell off with shine! New shine pos: %.1f, %.1f, %.1f\n", mInfo->shinePos.x, mInfo->shinePos.y, mInfo->shinePos.z);

            if (mModeLayout) {
                mModeLayout->showShineRespawned();
            }

            // Send FALLOFF packet - this will include the new shine position
            sendShineThiefPacket(ShineThiefUpdateType::FALLOFF);
        }
    } else {
        mRecoverySafetyPoint = sead::Vector3f::zero;
    }

    return true;
}

bool ShineThiefMode::tryEndRecoveryEvent() {
    if (!mWipeHolder)
        return false;

    mWipeHolder->startOpen(mRecoveryEventLength / 2);

    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if (!player)
        return false;

    // Remove the mInfo->mIsRound check - warp regardless of round state
    if (mRecoverySafetyPoint != sead::Vector3f::zero)
        warpToRecoveryPoint(player);

    player->endDemoPuppetable();

    if (!mIsEndgameActive)
        mModeLayout->hideEndgameScreen();

    return true;
}

void ShineThiefMode::warpToRecoveryPoint(PlayerActorHakoniwa* actor) {
    actor->startDemoPuppetable();
    // if (mPuppetHolder->getSize() > 0) {
    //     int randIdx = al::getRandom(mPuppetHolder->getSize());
    //     PuppetInfo* inf = Client::getPuppetInfo(randIdx);

    //     if (inf->isInSameStage && inf->isConnected) {
    //         al::setTrans(actor, inf->playerPos);
    //         return;
    //     }
    // }

    al::setTrans(actor, mRecoverySafetyPoint);
    actor->endDemoPuppetable();
}

void ShineThiefMode::tryStartEndgameEvent() {
    mIsEndgameActive = true;
    mEndgameTimer = 0.f;
    mModeLayout->showEndgameScreen();

    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if (!player)
        return;

    if (player->getPlayerHackKeeper()->mHackActor)
        player->getPlayerHackKeeper()->cancelHackArea();

    player->startDemoPuppetable();
    rs::faceToCamera(player);
    player->mAnimator->endSubAnim();

    if (mInfo->mIsPlayerHolder) {
        player->mAnimator->startAnim("RaceResultWin");
    } else {
        player->mAnimator->startAnim("RaceResultLose");
    }

    endRound(false);
}

bool ShineThiefMode::trySetPostProcessingType(ShineThiefPostProcessingType type) {
    u8 ppIdx = static_cast<u8>(type);
    u32 curIdx = al::getPostProcessingFilterPresetId(mCurScene);

    if (ppIdx == curIdx || !mCurScene)
        return false;

    if (type == ShineThiefPostProcessingType::PPDISABLED) {
        while (curIdx != ppIdx) {
            al::incrementPostProcessingFilterPreset(mCurScene);
            curIdx = (curIdx + 1) % 18;
        }
        al::invalidatePostProcessingFilter(mCurScene);
        return true;
    }

    while (curIdx != ppIdx) {
        al::incrementPostProcessingFilterPreset(mCurScene);
        curIdx = (curIdx + 1) % 18;
    }

    al::validatePostProcessingFilter(mCurScene);
    return true;
}

// In ShineThiefMode::updatePlayerConst() - Add warning trigger when applying const changes
void ShineThiefMode::updatePlayerConst() {
    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if (!player || !player->mConst)
        return;

    PlayerConst* playerConst = player->mConst;
    if (!playerConst)
        return;

    // Check if we should apply speed reduction (after 45 seconds)
    bool shouldApply = mInfo->mIsPlayerHolder && mInfo->mIsRound && mShineHoldTimer >= 45.f;

    if (shouldApply && !mIsConstApplied) {
        // Save original values
        mOriginalConst.mNormalMaxSpeed = player->mConst->mNormalMaxSpeed;
        mOriginalConst.mNormalMaxSpeed2D = player->mConst->mNormalMaxSpeed2D;
        mOriginalConst.mNormalMinSpeed = player->mConst->mNormalMinSpeed;
        mOriginalConst.mNormalMinSpeed2D = player->mConst->mNormalMinSpeed2D;
        mOriginalConst.mDashBorderSpeed = player->mConst->mDashBorderSpeed;
        mOriginalConst.mDashFastBorderSpeed = player->mConst->mDashFastBorderSpeed;
        mOriginalConst.mDashMaxSpeed2D = player->mConst->mDashMaxSpeed2D;
        mOriginalConst.mSlopeRollingMaxSpeed = player->mConst->mSlopeRollingMaxSpeed;
        mOriginalConst.mSlopeRollingSideMaxSpeed = player->mConst->mSlopeRollingSideMaxSpeed;
        mOriginalConst.mSlopeRollingSpeedBoost = player->mConst->mSlopeRollingSpeedBoost;
        mOriginalConst.mSlopeRollingSpeedEnd = player->mConst->mSlopeRollingSpeedEnd;
        mOriginalConst.mSlopeRollingSpeedStart = player->mConst->mSlopeRollingSpeedStart;
        mOriginalConst.mLongJumpSpeed = player->mConst->mLongJumpSpeed;
        mOriginalConst.mLongJumpSpeedMin = player->mConst->mLongJumpSpeedMin;
        mOriginalConst.mLongJumpInitSpeed = player->mConst->mLongJumpInitSpeed;

        // Apply speed reduction (50% of original)
        float mult = 0.5f;
        player->mConst->mNormalMaxSpeed *= mult;
        player->mConst->mNormalMaxSpeed2D *= mult;
        player->mConst->mNormalMinSpeed *= mult;
        player->mConst->mNormalMinSpeed2D *= mult;
        player->mConst->mDashBorderSpeed *= mult;
        player->mConst->mDashFastBorderSpeed *= mult;
        player->mConst->mDashMaxSpeed2D *= mult;
        player->mConst->mSlopeRollingMaxSpeed *= mult;
        player->mConst->mSlopeRollingSideMaxSpeed *= mult;
        player->mConst->mSlopeRollingSpeedBoost *= mult;
        player->mConst->mSlopeRollingSpeedEnd *= mult;
        player->mConst->mSlopeRollingSpeedStart *= mult;
        player->mConst->mLongJumpSpeed *= mult;
        player->mConst->mLongJumpSpeedMin *= mult;
        player->mConst->mLongJumpInitSpeed *= mult;

        mIsConstApplied = true;

        if (mModeLayout) {
            mModeLayout->show45SecondWarning();
        }
    } else if (!shouldApply && mIsConstApplied) {
        // Restore original values
        player->mConst->mNormalMaxSpeed = mOriginalConst.mNormalMaxSpeed;
        player->mConst->mNormalMaxSpeed2D = mOriginalConst.mNormalMaxSpeed2D;
        player->mConst->mNormalMinSpeed = mOriginalConst.mNormalMinSpeed;
        player->mConst->mNormalMinSpeed2D = mOriginalConst.mNormalMinSpeed2D;
        player->mConst->mDashBorderSpeed = mOriginalConst.mDashBorderSpeed;
        player->mConst->mDashFastBorderSpeed = mOriginalConst.mDashFastBorderSpeed;
        player->mConst->mDashMaxSpeed2D = mOriginalConst.mDashMaxSpeed2D;
        player->mConst->mSlopeRollingMaxSpeed = mOriginalConst.mSlopeRollingMaxSpeed;
        player->mConst->mSlopeRollingSideMaxSpeed = mOriginalConst.mSlopeRollingSideMaxSpeed;
        player->mConst->mSlopeRollingSpeedBoost = mOriginalConst.mSlopeRollingSpeedBoost;
        player->mConst->mSlopeRollingSpeedEnd = mOriginalConst.mSlopeRollingSpeedEnd;
        player->mConst->mSlopeRollingSpeedStart = mOriginalConst.mSlopeRollingSpeedStart;
        player->mConst->mLongJumpSpeed = mOriginalConst.mLongJumpSpeed;
        player->mConst->mLongJumpSpeedMin = mOriginalConst.mLongJumpSpeedMin;
        player->mConst->mLongJumpInitSpeed = mOriginalConst.mLongJumpInitSpeed;

        mIsConstApplied = false;
    }
}