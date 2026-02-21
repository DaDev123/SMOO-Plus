#include "server/shine-thief/ShineThiefMode.hpp"

#include "al/Library/Collision/CollisionPartsKeeperUtil.h"
#include "al/Library/Collision/CollisionPartsTriangle.h"
#include "al/Library/Controller/InputFunction.h"
#include "al/Library/LiveActor/ActorFlagFunction.h"
#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/Math/MathUtil.h"
#include "al/Library/Nature/NatureUtil.h"

#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Player/PlayerAnimator.h"
#include "game/Player/PlayerFunction.h"
#include "game/Player/PlayerHackKeeper.h"
#include "game/System/PlayerHitPointData.h"
#include "game/Util/ActorDimensionKeeper.h"
#include "game/Util/ObjUtil.h"

#include "actors/PuppetActor.h"
#include "imgui.h"
#include "Library/LiveActor/LiveActor.h"
#include "Library/Thread/AsyncFunctorThread.h"
#include "logger.hpp"
#include "packets/ShineThiefInf.h"
#include "puppets/PuppetInfo.h"
#include "server/Client.hpp"
#include "server/DeltaTime.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/shine-thief/StageBoundInfo.h"
#include "System/GameDataFile.h"
#include "System/GameDataFunction.h"
#include "System/GameDataHolder.h"
#include "System/GameDataHolderAccessor.h"

ShineThiefMode::ShineThiefMode(const char* name) : GameModeBase(name) {}

void ShineThiefMode::init(const GameModeInitInfo& info) {
    mSceneObjHolder = info.mSceneObjHolder;
    mMode = info.mMode;
    mCurScene = (StageScene*)info.mScene;
    mPuppetHolder = info.mPuppetHolder;

    GameModeInfoBase* curGameInfo = GameModeManager::instance()->getInfo<ShineThiefInfo>();

    sead::ScopedCurrentHeapSetter heapSetter(GameModeManager::instance()->getHeap());

    if (curGameInfo && curGameInfo->mMode == mMode) {
        mInfo = (ShineThiefInfo*)curGameInfo;
        mModeTimer = new GameModeTimer(mInfo->mRoundTimer);
    } else {
        if (curGameInfo)
            delete curGameInfo;
        mInfo = GameModeManager::instance()->createModeInfo<ShineThiefInfo>();
        mModeTimer = new GameModeTimer();
    }

    mInfo->mHolderPlayers.allocBuffer(0x10, al::getSceneHeap());
    mInfo->mThiefPlayers.allocBuffer(0x10, al::getSceneHeap());
    mInfo->mTeam1Players.allocBuffer(0x10, al::getSceneHeap());  // ADD
    mInfo->mTeam2Players.allocBuffer(0x10, al::getSceneHeap());  // ADD

    if (mBoundInfo)
        delete mBoundInfo;
    mBoundInfo = new StageBoundInfo();

    sead::ScopedCurrentHeapSetter heapSetterr(al::getSceneHeap());

    mModeLayout = new ShineThiefIcon("ShineThiefIcon", *info.mLayoutInitInfo);
    mInfo->mPlayerTagScore.setTargetLayout(mModeLayout);

    mMainPlayerCaptureBlock = new ShineThiefPlayerBlock("MainPlayerBlock");
    mMainPlayerCaptureBlock->init(*info.mActorInitInfo);

    mHintArrow = new ShineThiefHintArrow("ShineHolderHintArrow");
    mHintArrow->init(*info.mActorInitInfo);
}

void ShineThiefMode::sendShineThiefPacket(ShineThiefUpdateType updateType) {
    mNextUpdateType = updateType;
    Client::sendShineThiefInfPacket();
}

void ShineThiefMode::begin() {
    unpause();

    mInvulnTime = 0.f;
    mIsScoreEventsValid = true;

    mShineHoldTimer = 0.f;
    mIsConstApplied = false;

    if (mInfo->mIsRound)
        mModeTimer->enableTimer();
    mModeTimer->disableControl();
    mModeTimer->setTimerDirection(false);

    PlayerHitPointData* hit = GameDataHolderAccessor(mCurScene)->getGameDataFile()->getPlayerHitPointData();
    hit->mCurrentHealth = 3;

    sendShineThiefPacket(ShineThiefUpdateType::PLAYER);

    GameModeBase::begin();
    mCurScene->stageSceneLayout->end();
}

void ShineThiefMode::end() {
    pause();

    mInvulnTime = 0.f;
    mIsScoreEventsValid = false;

    mShineHoldTimer = 0.f;
    if (mIsConstApplied) {
        PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
        if (player && player->mConst) {
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

    mCurScene->stageSceneLayout->start();

    if (!GameModeManager::instance()->isPaused()) {
        if (mInfo->mIsPlayerHolder)
            trySetPlayerHolderState(false);

        if (al::isAlive(mMainPlayerCaptureBlock)) {
            mMainPlayerCaptureBlock->end();
        }
    }

    GameModeBase::end();
}

void ShineThiefMode::pause() {
    GameModeBase::pause();
    mModeLayout->tryEnd();
}

void ShineThiefMode::unpause() {
    GameModeBase::unpause();
    mModeLayout->appear();
}

void ShineThiefMode::update() {
    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if (!player)
        return;

    // Initialize stage bounds
    if (mBoundInfo)
        mBoundInfo->init(mMainPlayerCaptureBlock, 50000.f, 200.f);

    // Show player block if not locked and round is active
    if (!mMainPlayerCaptureBlock->mIsLocked && mInfo->mIsRound)
        mMainPlayerCaptureBlock->appear();

    // Handle round intro animation
    if (mIsRoundIntro) {
        updateRoundIntro();
        if (mIsRoundIntro)
            return;
    }

    // Update timer
    mModeTimer->updateTimer();
    mModeTimer->disableControl();

    // Award points every second while holding shine
    if ((mInfo->mRoundTimer.mSeconds != mModeTimer->getTime().mSeconds) && mInfo->mIsPlayerHolder && mInfo->mIsRound)
        mInfo->mPlayerTagScore.eventScoreSurvivalTime();

    mInfo->mRoundTimer = mModeTimer->getTime();

    // Check if round time expired
    if (mModeTimer->isEnabled() && mModeTimer->getTimeCombined() <= 0.f)
        tryStartEndgameEvent();

    // Update shine hold timer
    if (mInfo->mIsPlayerHolder && mInfo->mIsRound) {
        mShineHoldTimer += Time::deltaTime;
    } else {
        mShineHoldTimer = 0.f;
    }

    // Update player movement constants based on shine hold time
    updatePlayerConst();

    // Update player lists
    mInfo->mHolderPlayers.clear();
    mInfo->mThiefPlayers.clear();

    if (mInfo->mIsTeamMode) {
        mInfo->mTeam1Players.clear();
        mInfo->mTeam2Players.clear();
    }

    for (int i = 0; i < mPuppetHolder->getSize(); i++) {
        PuppetInfo* curInfo = Client::getPuppetInfo(i);
        if (!curInfo->isConnected)
            continue;

        // Update holder/thief lists
        if (curInfo->isShineThiefHolder)
            mInfo->mHolderPlayers.pushBack(curInfo);
        else
            mInfo->mThiefPlayers.pushBack(curInfo);

        // Update team lists if in team mode
        if (mInfo->mIsTeamMode) {
            if (curInfo->shineThiefTeam == 1)
                mInfo->mTeam1Players.pushBack(curInfo);
            else if (curInfo->shineThiefTeam == 2)
                mInfo->mTeam2Players.pushBack(curInfo);
        }
    }

    // Calculate team scores
    if (mInfo->mIsTeamMode) {
        mInfo->mTeam1Score = 0;
        mInfo->mTeam2Score = 0;

        // Add player's score to their team
        uint8_t playerTeam = (uint8_t)mInfo->mPlayerTeam;
        if (playerTeam == 1) {
            mInfo->mTeam1Score += mInfo->mPlayerTagScore.mScore;
        } else if (playerTeam == 2) {
            mInfo->mTeam2Score += mInfo->mPlayerTagScore.mScore;
        }

        // Add all puppet scores to their teams
        for (int i = 0; i < mPuppetHolder->getSize(); i++) {
            PuppetInfo* curInfo = Client::getPuppetInfo(i);
            if (!curInfo->isConnected)
                continue;

            if (curInfo->shineThiefTeam == 1) {
                mInfo->mTeam1Score += curInfo->shineThiefScore;
            } else if (curInfo->shineThiefTeam == 2) {
                mInfo->mTeam2Score += curInfo->shineThiefScore;
            }
        }
    }

    // Increment invulnerability timer
    mInvulnTime += Time::deltaTime;

    // Find closest shine holder for arrow and check for shine stealing
    float closeHolderDistance = 9999999.f;
    PuppetInfo* closeHolder = nullptr;
    PuppetActor* closeHolderPup = nullptr;
    bool stoleFromPlayer = false;

    if (mInfo->mIsRound && mInvulnTime >= 1.5f) {
        bool isPDead = PlayerFunction::isPlayerDeadStatus(player);
        bool isP2D = player->mDimensionKeeper->mIs2D;

        for (int i = 0; i < mPuppetHolder->getSize(); i++) {
            PuppetInfo* curInfo = Client::getPuppetInfo(i);
            PuppetActor* curPup = Client::getPuppet(i);

            if (!curInfo->isConnected || !curInfo->isInSameStage)
                continue;

            float pupDist = al::calcDistance(player, curPup);

            // Track closest holder for arrow
            if (pupDist < closeHolderDistance && curInfo->isShineThiefHolder) {
                closeHolderDistance = pupDist;
                closeHolder = curInfo;
                closeHolderPup = curPup;
            }

            // Check if we can steal from this puppet
            sead::Vector3f puppetShinePos = al::getTrans(curPup);
            puppetShinePos.y += 275.f;
            sead::Vector3f diff = al::getTrans(player) - puppetShinePos;
            float shineDist = diff.length();

            if (!mInfo->mIsPlayerHolder && (pupDist < 200.f || shineDist < 200.f) && isP2D == curInfo->is2D && !isPDead && curInfo->isShineThiefHolder) {
                bool canSteal = true;

                // Apply team restrictions
                if (mInfo->mIsTeamMode) {
                    uint8_t myTeam = (uint8_t)mInfo->mPlayerTeam;
                    uint8_t holderTeam = curInfo->shineThiefTeam;

                    // Cannot steal if on the same team (both must have teams assigned)
                    if (myTeam != 0 && holderTeam != 0 && myTeam == holderTeam) {
                        canSteal = false;
                    }
                }

                if (canSteal) {
                    tryStealShine();
                    stoleFromPlayer = true;
                    break;
                }
            }
        }

        // Check if anyone is holding the shine
        bool anyoneHoldingShine = mInfo->mIsPlayerHolder || closeHolder != nullptr;

        // If no one is holding the shine, check if player can pick it up from ground
        if (!stoleFromPlayer && !anyoneHoldingShine) {
            float shineDist = al::calcDistance(player, mInfo->shinePos);
            if (shineDist < 200.f && !isPDead) {
                // When shine is on ground, anyone can pick it up regardless of team
                tryStealShine();
            }
        }
    }

    // Update hint arrow
    if (mInfo->mIsPlayerHolder)
        mHintArrow->setTarget(nullptr);
    else if (!closeHolder || !closeHolderPup)
        mHintArrow->setTarget(&mInfo->shinePos);
    else
        mHintArrow->setTarget(al::getTransPtr(closeHolderPup));

    // Recovery event countdown
    if (mRecoveryEventFrames > 0) {
        mRecoveryEventFrames--;
        if (mRecoveryEventFrames == 0)
            tryEndRecoveryEvent();
    }

    // Endgame event
    if (mIsEndgameActive) {
        mEndgameTimer += Time::deltaTime;
        if (mEndgameTimer > 6.f) {
            mInfo->mIsPlayerHolder = false;
            mInvulnTime = 0.f;
            sendShineThiefPacket(ShineThiefUpdateType::PLAYER);
            mIsEndgameActive = false;
            tryStartRecoveryEvent(true);
        }
    }

    // Sync score to other clients when it changes
    ShineThiefScore* score = &mInfo->mPlayerTagScore;
    if (score->mScore != score->mPrevScore) {
        score->mPrevScore = score->mScore;
        sendShineThiefPacket(ShineThiefUpdateType::PLAYER);
    }

    // Update player's shine block visual
    if (mInfo->mIsPlayerHolder && mInfo->mIsRound) {
        if (!al::isAlive(mMainPlayerCaptureBlock))
            mMainPlayerCaptureBlock->appear();

        sead::Vector3f offsetPos = al::getTrans(player);
        offsetPos.y += 275.f;
        setShinePos(offsetPos);
        al::setTrans(mMainPlayerCaptureBlock, getShinePos());
        al::setQuat(mMainPlayerCaptureBlock, al::getQuat(player));
    } else {
        if (al::isAlive(mMainPlayerCaptureBlock) && mMainPlayerCaptureBlock->mIsLocked)
            al::setTrans(mMainPlayerCaptureBlock, getShinePos());
    }

    // Controls - Reset score
    if (al::isPadTriggerDown(-1) && al::isPadHoldL(-1) && mRecoveryEventFrames == 0 && !mIsEndgameActive)
        mInfo->mPlayerTagScore.resetScore();

    // Host Controls - Start round
    if (al::isPadTriggerUp(-1) && al::isPadHoldR(-1) && mInfo->mIsHostMode && !mInfo->mIsRound) {
        placeShine();
        startRound(mInfo->mRoundLength);
        al::setTrans(mMainPlayerCaptureBlock, mInfo->shinePos);
        sendShineThiefPacket(ShineThiefUpdateType::ROUNDSTART);
    }

    // Host Controls - End round
    if (al::isPadTriggerDown(-1) && al::isPadHoldR(-1) && mInfo->mIsHostMode && mInfo->mIsRound) {
        endRound(true);
        sendShineThiefPacket(ShineThiefUpdateType::ROUNDCANCEL);
    }

    // Debug controls
    if (mInfo->mIsDebugMode) {
        if (al::isPadTriggerRight(-1) && al::isPadHoldX(-1))
            trySetPlayerHolderState(true);
        if (al::isPadTriggerRight(-1) && al::isPadHoldY(-1))
            trySetPlayerHolderState(false);
        if (al::isPadTriggerRight(-1) && al::isPadHoldA(-1))
            mInfo->mPlayerTagScore.eventScoreDebug();
        if (al::isPadTriggerRight(-1) && al::isPadHoldB(-1))
            tryStartEndgameEvent();
        if (al::isPadTriggerLeft(-1) && al::isPadHoldA(-1))
            mModeTimer->setTime(0.f, 5, 1, 0);
    }

    // Hide stage UI
    if (!mCurScene->stageSceneLayout->isEnd())
        mCurScene->stageSceneLayout->end();
}

void ShineThiefMode::debugMenuControls() {
    ImGui::Text("- L + ← | Enable/disable Shine Thief\n");
    ImGui::Text("- L + ↓ | Reset score\n");

    if (mInfo->mIsHostMode) {
        ImGui::Text("- [Host] R + ↑ | Start round\n");
        ImGui::Text("- [Host] R + ↓ | End round\n");
    }

    if (mInfo->mIsDebugMode) {
        ImGui::Text("- [Debug] A + → | +Score\n");
        ImGui::Text("- [Debug] A + ← | Set time\n");
        ImGui::Text("- [Debug] B + → | End round\n");
        ImGui::Text("- [Debug] X + → | Get shine\n");
        ImGui::Text("- [Debug] Y + → | Drop shine\n");
    }
}

PlayerActorHakoniwa* ShineThiefMode::getPlayerActorHakoniwa() {
    PlayerActorBase* playerBase = (PlayerActorBase*)rs::getPlayerActor(mCurScene);
    if (!playerBase || !playerBase->getPlayerInfo())
        return nullptr;
    return (PlayerActorHakoniwa*)playerBase;
}

void ShineThiefMode::placeShine() {
    if (!mBoundInfo) {
        Logger::log("Player placer run with no bound info?\n");
        return;
    }

    al::LiveActor* p1 = mMainPlayerCaptureBlock;
    sead::Vector3f resultVec = sead::Vector3f::zero;
    al::Triangle resultTri;
    float resultAngle = 0.f;
    int attempts = 0;
    bool foundPos = false;

    while (!foundPos) {
        sead::Vector3f checkOrigin = sead::Vector3f::zero;
        checkOrigin.x = al::getRandom(mBoundInfo->mMin.x, mBoundInfo->mMax.x);
        checkOrigin.y = mBoundInfo->mPeakHeight;
        checkOrigin.z = al::getRandom(mBoundInfo->mMin.z, mBoundInfo->mMax.z);

        sead::Vector3f ray = {0.f, -mBoundInfo->mPeakHeight * 2, 0.f};

        foundPos = alCollisionUtil::getFirstPolyOnArrow(p1, &resultVec, &resultTri, checkOrigin, ray, nullptr, nullptr);

        if (foundPos) {
            resultAngle = al::calcAngleDegree(sead::Vector3f::ey, resultTri.getFaceNormal());

            if (resultAngle > 45.f || al::isFloorCode(resultTri, "Needle") || al::isFloorCode(resultTri, "Poison") || al::isFloorCode(resultTri, "LavaPink") ||
                al::isFloorCode(resultTri, "DamageFire") || al::isFloorCode(resultTri, "Slide"))
                foundPos = false;
        }

        if (!foundPos && ++attempts > 1000) {
            resultVec = al::getTrans(p1);
            foundPos = true;
        }
    }

    Logger::log("Found warp at x:%.01f y:%.01f z:%.01f (angle: %f)\n", resultVec.x, resultVec.y, resultVec.z, resultAngle);
    resultVec.y += 100.f;
    setShinePos(resultVec);
}
