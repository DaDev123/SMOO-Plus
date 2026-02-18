#include "layouts/ShineThiefIcon.h"

#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/Math/MathUtil.h"
#include "al/Library/Nerve/NerveUtil.h"

#include <cstring>

#include "layouts/ShineThiefRunnerSlot.h"
#include "server/DeltaTime.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/shine-thief/ShineThiefInfo.h"

ShineThiefIcon::ShineThiefIcon(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {
    al::initLayoutActor(this, initInfo, "ShineThiefIcon", 0);
    al::hidePane(this, "Endgame");

    // Hide countdown panes initially
    hideCountdown();

    // Set default countdown scale to 0.6
    mCountdownScale = 0.6f;
    mCountdownFrames = 0;
    mCountdownScalingDown = false;

    // Set initial scale for all countdown panes
    al::setPaneLocalScale(this, "Countdown_3", {0.6f, 0.6f});
    al::setPaneLocalScale(this, "Countdown_2", {0.6f, 0.6f});
    al::setPaneLocalScale(this, "Countdown_1", {0.6f, 0.6f});
    al::setPaneLocalScale(this, "Countdown_GO", {0.6f, 0.6f});

    mWarning45sScale = 0.0f;
    mWarning45sTimer = -1.0f;
    al::hidePane(this, "TxtEvent");
    al::setPaneLocalScale(this, "TxtEvent", {0.0f, 0.0f});

    mShineRespawnScale = 0.0f;
    mShineRespawnTimer = -1.0f;
    al::hidePane(this, "ShineRespawned");
    al::setPaneLocalScale(this, "ShineRespawned", {0.0f, 0.0f});

    al::hidePane(this, "Team1");
    al::hidePane(this, "Team2");

    mInfo = GameModeManager::instance()->getInfo<ShineThiefInfo>();
    mIsRunner = mInfo->mIsPlayerHolder;

    mRunnerSlots.tryAllocBuffer(mMaxRunners, al::getSceneHeap());
    for (int i = 0; i < mMaxRunners; i++) {
        ShineThiefRunnerSlot* newSlot = new (al::getSceneHeap()) ShineThiefRunnerSlot("RunnerSlot", initInfo);
        newSlot->init(i);
        mRunnerSlots.pushBack(newSlot);
    }

    mChaserSlots.tryAllocBuffer(mMaxChasers, al::getSceneHeap());
    for (int i = 0; i < mMaxChasers; i++) {
        ShineThiefChaserSlot* newSlot = new (al::getSceneHeap()) ShineThiefChaserSlot("ChaserSlot", initInfo);
        newSlot->init(i);
        mChaserSlots.pushBack(newSlot);
    }

    mSpectateName = nullptr;
    mScoreEventScale = 0.0f;
    al::setPaneLocalScale(this, "ScoreEvent", {0.0f, 0.0f});

    al::hidePane(this, "TxtTeam1Score");
    al::hidePane(this, "TxtTeam2Score");
    al::hidePane(this, "GoldBorder");
    al::hidePane(this, "BlueBorder");
    al::hidePane(this, "RedBorder");

    initNerve(&NrvShineThiefIcon.End, 0);
    kill();
}

void ShineThiefIcon::show45SecondWarning() {
    al::setPaneStringFormat(this, "TxtEvent", "Slowing you Down,\nyou've had the shine for more than 45 Seconds!");
    al::showPane(this, "TxtEvent");
    mWarning45sTimer = 0.0f;
    mWarning45sScale = 1.5f;
}

void ShineThiefIcon::showShineRespawned() {
    al::showPane(this, "ShineRespawned");
    mShineRespawnTimer = 0.0f;
    mShineRespawnScale = 0.75f;
}

void ShineThiefIcon::showCountdown(int number) {
    hideCountdown();

    mCountdownScale = 0.6f;
    mCountdownFrames = 0;
    mCountdownScalingDown = false;

    switch (number) {
    case 3:
        al::showPane(this, "Countdown_3");
        break;
    case 2:
        al::showPane(this, "Countdown_2");
        break;
    case 1:
        al::showPane(this, "Countdown_1");
        break;
    case 0:
        al::showPane(this, "Countdown_GO");
        break;
    }
}

void ShineThiefIcon::startCountdownScaleDown() {
    mCountdownScalingDown = true;
}

void ShineThiefIcon::hideCountdown() {
    al::hidePane(this, "Countdown_3");
    al::hidePane(this, "Countdown_2");
    al::hidePane(this, "Countdown_1");
    al::hidePane(this, "Countdown_GO");
}

void ShineThiefIcon::appear() {
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &NrvShineThiefIcon.Appear);

    for (int i = 0; i < mMaxRunners; i++)
        mRunnerSlots.at(i)->tryStart();

    for (int i = 0; i < mMaxChasers; i++)
        mChaserSlots.at(i)->tryStart();

    al::LayoutActor::appear();
}

bool ShineThiefIcon::tryEnd() {
    if (!al::isNerve(this, &NrvShineThiefIcon.End)) {
        al::setNerve(this, &NrvShineThiefIcon.End);

        for (int i = 0; i < mMaxRunners; i++)
            mRunnerSlots.at(i)->tryEnd();

        for (int i = 0; i < mMaxChasers; i++)
            mChaserSlots.at(i)->tryEnd();

        hideCountdown();

        return true;
    }
    return false;
}

bool ShineThiefIcon::tryStart() {
    if (!al::isNerve(this, &NrvShineThiefIcon.Wait) && !al::isNerve(this, &NrvShineThiefIcon.Appear)) {
        appear();
        return true;
    }

    return false;
}

void ShineThiefIcon::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &NrvShineThiefIcon.Wait);
    }
}

void ShineThiefIcon::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    // Set all overlay positions
    setSpectateOverlayHeight();
    setRoundTimerOverlay();
    updateTeamDisplay();
    updateHolderBorder();

    // Handle 45 second warning animation
    if (mWarning45sTimer >= 0.0f) {
        mWarning45sTimer += Time::deltaTime;

        if (mWarning45sTimer <= 0.2f) {
            mWarning45sScale = al::lerpValue(mWarning45sScale, 1.0f, 0.3f);
        } else if (mWarning45sTimer >= 3.0f) {
            mWarning45sScale = al::lerpValue(mWarning45sScale, 0.0f, 0.15f);

            if (mWarning45sScale < 0.05f) {
                al::hidePane(this, "TxtEvent");
                mWarning45sTimer = -1.0f;
            }
        }

        al::setPaneLocalScale(this, "TxtEvent", {mWarning45sScale, mWarning45sScale});
    }

    // Handle shine respawn animation
    if (mShineRespawnTimer >= 0.0f) {
        mShineRespawnTimer += Time::deltaTime;

        if (mShineRespawnTimer <= 0.15f) {
            mShineRespawnScale = al::lerpValue(mShineRespawnScale, 0.55f, 0.35f);
        } else if (mShineRespawnTimer <= 0.3f) {
            mShineRespawnScale = al::lerpValue(mShineRespawnScale, 0.5f, 0.25f);
        } else if (mShineRespawnTimer >= 3.5f) {
            mShineRespawnScale = al::lerpValue(mShineRespawnScale, 0.0f, 0.12f);

            if (mShineRespawnScale < 0.05f) {
                al::hidePane(this, "ShineRespawned");
                mShineRespawnTimer = -1.0f;
            }
        }

        al::setPaneLocalScale(this, "ShineRespawned", {mShineRespawnScale, mShineRespawnScale});
    }

    // Animate countdown with pop effect
    bool anyCountdownVisible = !al::isHidePane(this, "Countdown_3") || !al::isHidePane(this, "Countdown_2") || !al::isHidePane(this, "Countdown_1") ||
                               !al::isHidePane(this, "Countdown_GO");

    if (anyCountdownVisible) {
        mCountdownFrames++;

        if (mCountdownScalingDown) {
            mCountdownScale = al::lerpValue(mCountdownScale, 0.0f, 0.1f);

            if (mCountdownScale < 0.05f) {
                hideCountdown();
                mCountdownScalingDown = false;
            }
        } else {
            if (mCountdownFrames <= 8) {
                mCountdownScale = al::lerpValue(mCountdownScale, 0.84f, 0.35f);
            } else if (mCountdownFrames <= 18) {
                mCountdownScale = al::lerpValue(mCountdownScale, 0.6f, 0.25f);
            } else {
                mCountdownScale = 0.6f;
            }
        }

        al::setPaneLocalScale(this, "Countdown_3", {mCountdownScale, mCountdownScale});
        al::setPaneLocalScale(this, "Countdown_2", {mCountdownScale, mCountdownScale});
        al::setPaneLocalScale(this, "Countdown_1", {mCountdownScale, mCountdownScale});
        al::setPaneLocalScale(this, "Countdown_GO", {mCountdownScale, mCountdownScale});
    } else {
        mCountdownFrames = 0;
        mCountdownScalingDown = false;
    }

    // Update score event info
    if (mScoreEventIsQueued) {
        mScoreEventIsQueued = false;
        al::setPaneStringFormat(this, "TxtScoreNum", "%i", mScoreEventValue);
        al::setPaneStringFormat(this, "TxtScoreDesc", "%s", mScoreEventDesc);
        mScoreEventScale = 1.5f;
    }

    if (mScoreEventTime >= 0.f) {
        mScoreEventTime += Time::deltaTime;

        if (mScoreEventTime > 3.75f)
            mScoreEventValue = 0;

        float targetScale = mScoreEventTime < 3.f ? 1.0f : 0.f;
        mScoreEventScale = al::lerpValue(mScoreEventScale, targetScale, 0.15f);
        al::setPaneLocalScale(this, "ScoreEvent", {mScoreEventScale, mScoreEventScale});
    }

    // Endgame UI
    if (mEndgameIsDisplay) {
        if (al::isHidePane(this, "Endgame"))
            al::showPane(this, "Endgame");
    }

    if (!mEndgameIsDisplay && !al::isHidePane(this, "Endgame"))
        al::hidePane(this, "Endgame");

    // Handle endgame text pop animation
    if (mEndgameTextTimer >= 0.0f) {
        mEndgameTextTimer += Time::deltaTime;

        if (mEndgameTextTimer <= 0.15f) {
            mEndgameTextScale = al::lerpValue(mEndgameTextScale, 0.55f, 0.35f);
        } else if (mEndgameTextTimer <= 0.3f) {
            mEndgameTextScale = al::lerpValue(mEndgameTextScale, 0.5f, 0.25f);
        } else if (mEndgameTextTimer >= 3.5f) {
            mEndgameTextScale = al::lerpValue(mEndgameTextScale, 0.0f, 0.12f);

            if (mEndgameTextScale < 0.08f) {
                mEndgameTextScale = 0.0f;
                al::setPaneLocalScale(this, "PicEndgameText", {0.0f, 0.0f});
                mEndgameTextTimer = -1.0f;
            }
        }

        al::setPaneLocalScale(this, "PicEndgameText", {mEndgameTextScale, mEndgameTextScale});
    }
}

void ShineThiefIcon::queueScoreEvent(int eventValue, const char* eventDesc) {
    mScoreEventTime = 0.f;
    mScoreEventIsQueued = true;
    mScoreEventValue += eventValue;
    mScoreEventValue = al::clamp(mScoreEventValue, 0, 9999);

    mScoreEventDesc = eventDesc;
    mScoreEventScale = 1.35f;
}

void ShineThiefIcon::setSpectateOverlayHeight() {
    float targetHeight = -400.f;
    mSpectateOverlayHeight = al::lerpValue(mSpectateOverlayHeight, targetHeight, 0.04f);
    al::setPaneLocalTrans(this, "Spectate", {0.f, mSpectateOverlayHeight, 0.f});
}

void ShineThiefIcon::setRoundTimerOverlay() {
    float targetHeight = mInfo->mIsRound && !mEndgameIsDisplay ? 330.f : 390.f;
    mRoundTimerHeight = al::lerpValue(mRoundTimerHeight, targetHeight, 0.03f);
    al::setPaneLocalTrans(this, "RoundTimer", {0.f, mRoundTimerHeight, 0.f});

    float targetScale = mInfo->mIsRound && !mEndgameIsDisplay && mInfo->mRoundTimer.mMinutes <= 0 ? 1.66f : 1.f;
    mRoundTimerScale = al::lerpValue(mRoundTimerScale, targetScale, 0.02f);
    al::setPaneLocalScale(this, "RoundTimer", {mRoundTimerScale, mRoundTimerScale});

    if (mInfo->mIsRound) {
        mRoundTimerClockInsideSpin -= 1.2f;
        if (mRoundTimerClockInsideSpin < -360.f)
            mRoundTimerClockInsideSpin += 360.f;

        al::setPaneLocalRotate(this, "PicRoundTimerSpin", {0.f, 0.f, mRoundTimerClockInsideSpin});
    }

    al::setPaneStringFormat(this, "TxtRoundTimer", "%02i:%02i", mInfo->mRoundTimer.mMinutes, mInfo->mRoundTimer.mSeconds);
}

void ShineThiefIcon::updateHolderBorder() {
    bool isHolder = mInfo->mIsPlayerHolder;

    // Determine which border panes to show/hide
    bool showGold = false;
    bool showBlue = false;
    bool showRed = false;

    if (isHolder) {
        if (mInfo->mIsTeamMode) {
            if (mInfo->mPlayerTeam == ShineThiefTeam::TEAM_1)
                showBlue = true;
            else
                showRed = true;
        } else {
            showGold = true;
        }
    }

    // Show/hide each border pane
    if (showGold)
        al::showPane(this, "GoldBorder");
    else
        al::hidePane(this, "GoldBorder");

    if (showBlue)
        al::showPane(this, "BlueBorder");
    else
        al::hidePane(this, "BlueBorder");

    if (showRed)
        al::showPane(this, "RedBorder");
    else
        al::hidePane(this, "RedBorder");

    // Pulse scale between 1.0 and 1.02 continuously
    if (isHolder) {
        mBorderPulseTimer += Time::deltaTime;

        float pulse = 1.0f + 0.01f * (1.0f + sinf(mBorderPulseTimer * 3.0f));

        const char* activeBorder = showGold ? "GoldBorder" : (showBlue ? "BlueBorder" : "RedBorder");
        al::setPaneLocalScale(this, activeBorder, {pulse, pulse});
    } else {
        mBorderPulseTimer = 0.0f;
    }
}

void ShineThiefIcon::updateTeamDisplay() {
    if (mInfo->mIsTeamMode) {
        // Show both team headers in team mode
        al::showPane(this, "Team1");
        al::showPane(this, "Team2");
        al::showPane(this, "TxtTeam1Score");
        al::showPane(this, "TxtTeam2Score");

        // Hide the regular player list header
        al::hidePane(this, "PicHeaderChaser");

        // Update team scores
        al::setPaneStringFormat(this, "TxtTeam1Score", "%04u", mInfo->mTeam1Score);
        al::setPaneStringFormat(this, "TxtTeam2Score", "%04u", mInfo->mTeam2Score);

    } else {
        // Hide team headers when not in team mode
        al::hidePane(this, "Team1");
        al::hidePane(this, "Team2");

        al::hidePane(this, "TxtTeam1Score");
        al::hidePane(this, "TxtTeam2Score");

        // Show the regular player list header
        al::showPane(this, "PicHeaderChaser");
    }
}

void ShineThiefIcon::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}