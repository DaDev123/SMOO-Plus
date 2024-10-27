#include "server/coinrunners/CoinRunnerIcon.h"

#include "al/string/StringTmp.h"
#include "al/util.hpp"

#include "server/DeltaTime.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/coinrunners/CoinRunnerInfo.h"
#include "server/coinrunners/CoinRunnerRunnerSlot.h"
#include "server/coinrunners/CoinRunnerChaserSlot.h"
#include "server/coinrunners/CoinRunnerOtherSlot.h"

CoinRunnerIcon::CoinRunnerIcon(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {
    al::initLayoutActor(this, initInfo, "CoinRunnerIcon", 0);
    al::hidePane(this, "Endgame");

    mInfo     = GameModeManager::instance()->getInfo<CoinRunnerInfo>();
    mIsRunner = mInfo->isPlayerRunner();

    mRunnerSlots.tryAllocBuffer(mMaxRunners, al::getSceneHeap());
    for (int i = 0; i < mMaxRunners; i++) {
        CoinRunnerRunnerSlot* newSlot = new (al::getSceneHeap()) CoinRunnerRunnerSlot("RunnerSlot", initInfo);
        newSlot->init(i);
        mRunnerSlots.pushBack(newSlot);
    }

    mChaserSlots.tryAllocBuffer(mMaxChasers, al::getSceneHeap());
    for (int i = 0; i < mMaxChasers; i++) {
        CoinRunnerChaserSlot* newSlot = new (al::getSceneHeap()) CoinRunnerChaserSlot("ChaserSlot", initInfo);
        newSlot->init(i);
        mChaserSlots.pushBack(newSlot);
    }

    mOtherSlots.tryAllocBuffer(mMaxOthers, al::getSceneHeap());
    for (int i = 0; i < mMaxOthers; i++) {
        CoinRunnerOtherSlot* newSlot = new (al::getSceneHeap()) CoinRunnerOtherSlot("OtherSlot", initInfo);
        newSlot->init(i);
        mOtherSlots.pushBack(newSlot);
    }

    mSpectateName = nullptr;

    initNerve(&nrvCoinRunnerIconEnd, 0);
    kill();
}

void CoinRunnerIcon::appear() {
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &nrvCoinRunnerIconAppear);

    for (int i = 0; i < mMaxRunners; i++) {
        mRunnerSlots.at(i)->tryStart();
    }

    for (int i = 0; i < mMaxChasers; i++) {
        mChaserSlots.at(i)->tryStart();
    }

    for (int i = 0; i < mMaxOthers; i++) {
        mOtherSlots.at(i)->tryStart();
    }

    al::LayoutActor::appear();
}

bool CoinRunnerIcon::tryEnd() {
    if (!al::isNerve(this, &nrvCoinRunnerIconEnd)) {
        al::setNerve(this, &nrvCoinRunnerIconEnd);

        for (int i = 0; i < mMaxRunners; i++) {
            mRunnerSlots.at(i)->tryEnd();
        }

        for (int i = 0; i < mMaxChasers; i++) {
            mChaserSlots.at(i)->tryEnd();
        }

        for (int i = 0; i < mMaxOthers; i++) {
            mOtherSlots.at(i)->tryEnd();
        }

        return true;
    }
    return false;
}

bool CoinRunnerIcon::tryStart() {
    if (!al::isNerve(this, &nrvCoinRunnerIconWait) && !al::isNerve(this, &nrvCoinRunnerIconAppear)) {
        appear();
        return true;
    }
    return false;
}

void CoinRunnerIcon::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvCoinRunnerIconWait);
    }
}

void CoinRunnerIcon::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    // Set all overlay positons
    setFreezeOverlayHeight();
    setSpectateOverlayHeight();
    setRoundTimerOverlay();

    // Update score event info
    if (mScoreEventIsQueued) {
        mScoreEventIsQueued = false;
        al::setPaneStringFormat(this, "TxtScoreNum", "%i", mScoreEventValue);
        al::setPaneStringFormat(this, "TxtScoreDesc", "%s", mScoreEventDesc);
    }

    if (mScoreEventTime >= 0.f) {
        mScoreEventTime += Time::deltaTime;

        if (mScoreEventValue != 0) {
            al::setPaneLocalScale(this, "TxtScoreNum",  { 1.f, 1.f });
            al::setPaneLocalScale(this, "PicScorePlus", { 1.f, 1.f });
        } else {
            al::setPaneLocalScale(this, "TxtScoreNum",  { 0.f, 0.f });
            al::setPaneLocalScale(this, "PicScorePlus", { 0.f, 0.f });
        }

        if (mScoreEventTime > 3.75f) {
            mScoreEventValue = 0;
        }

        // Calculate score event pane's position
        sead::Vector3f targetPos = (
            mScoreEventTime < 3.f
            ? sead::Vector3f(   0.f, 235.f, 0.f)
            : sead::Vector3f(-650.f, 420.f, 0.f)
        );
        if (mInfo->isPlayerChaser()) {
            targetPos.x *= -1.f;
        }

        al::lerpVec(&mScoreEventPos, mScoreEventPos, targetPos, 0.05f);
        al::setPaneLocalTrans(this, "ScoreEvent", mScoreEventPos);

        // Calculate score event pane's scale
        float targetScale = mScoreEventTime < 3.f ? 1.0f : 0.f;
        mScoreEventScale = al::lerpValue(mScoreEventScale, targetScale, 0.05f);
        al::setPaneLocalScale(this, "ScoreEvent", { mScoreEventScale, mScoreEventScale });
    }

    // Spectate UI
    if (mInfo->isPlayerFrozen() && mSpectateName) {
        al::setPaneStringFormat(this, "TxtSpectateTarget", "%s", mSpectateName);
    }

    // Endgame UI (Wipeout)
    if (mEndgameIsDisplay) {
        if (al::isHidePane(this, "Endgame")) {
            al::showPane(this, "Endgame");
        }

        mEndgameTextSize  = al::lerpValue(mEndgameTextSize, 1.1f, 0.02f);
        mEndgameTextAngle = al::lerpValue(mEndgameTextAngle, 5.f, 0.01f);

        al::setPaneLocalScale(this, "PicEndgameText", { mEndgameTextSize, mEndgameTextSize });
        al::setPaneLocalRotate(this, "PicEndgameText", { 0.f, 0.f, mEndgameTextAngle });
    }

    if (!mEndgameIsDisplay && !al::isHidePane(this, "Endgame")) {
        al::hidePane(this, "Endgame");
    }

    // Other Players
    if (mInfo->isRound() || mInfo->others() == 0) {
        if (!al::isHidePane(this, "PicHeaderOther")) {
            al::hidePane(this, "PicHeaderOther");
        }
    } else {
        if (al::isHidePane(this, "PicHeaderOther")) {
            al::showPane(this, "PicHeaderOther");
        }
    }
}

void CoinRunnerIcon::queueScoreEvent(int eventValue, const char* eventDesc) {
    mScoreEventTime      = 0.f;
    mScoreEventIsQueued  = true;
    mScoreEventValue    += eventValue;
    mScoreEventValue     = al::clamp(mScoreEventValue, 0, 99);

    mScoreEventDesc  = eventDesc;
    mScoreEventPos   = sead::Vector3f(0.f, 245.f, 0.f);
    mScoreEventScale = 1.35f;
}

void CoinRunnerIcon::setFreezeOverlayHeight() {
    // Show or hide the frozen UI overlay (frozen borders on top and bottom of the screen when being frozen)
    float targetHeight = mInfo->isPlayerFrozen() ? 360.f : 415.f;
    mFreezeOverlayHeight = al::lerpValue(mFreezeOverlayHeight, targetHeight, 0.08f);
    al::setPaneLocalTrans(this, "PicFreezeOverlayTop", { 0.f, mFreezeOverlayHeight + 15.f, 0.f });
    al::setPaneLocalTrans(this, "PicFreezeOverlayBot", { 0.f, -mFreezeOverlayHeight, 0.f });
}

void CoinRunnerIcon::setSpectateOverlayHeight() {
    // Show or hide the spectator UI
    float targetHeight = (
           mInfo->isPlayerFrozen()
        && mInfo->runners() > 1
        && !mEndgameIsDisplay
        ? -250.f
        : -400.f
    );
    mSpectateOverlayHeight = al::lerpValue(mSpectateOverlayHeight, targetHeight, 0.04f);
    al::setPaneLocalTrans(this, "Spectate", { 0.f, mSpectateOverlayHeight, 0.f });
}

void CoinRunnerIcon::setRoundTimerOverlay() {
    // If round is active, set the timer's height on screen
    float targetHeight = (
           mInfo->isRound()
        && !mEndgameIsDisplay
        ? 330.f
        : 390.f
    );
    mRoundTimerHeight = al::lerpValue(mRoundTimerHeight, targetHeight, 0.03f);
    al::setPaneLocalTrans(this, "RoundTimer", { 0.f, mRoundTimerHeight, 0.f });

    // If time remaining is less than one minute, scale it up to be larger
    float targetScale = (
           mInfo->isRound()
        && !mEndgameIsDisplay
        && mInfo->mRoundTimer.mMinutes <= 0
        ? 1.66f
        : 1.f
    );
    mRoundTimerScale = al::lerpValue(mRoundTimerScale, targetScale, 0.02f);
    al::setPaneLocalScale(this, "RoundTimer", { mRoundTimerScale, mRoundTimerScale });

    // Spin the inside of the clock if a round is going
    if (mInfo->isRound()) {
        mRoundTimerClockInsideSpin -= 1.2f;
        if (mRoundTimerClockInsideSpin < -360.f) {
            mRoundTimerClockInsideSpin += 360.f;
        }

        al::setPaneLocalRotate(this, "PicRoundTimerSpin", { 0.f, 0.f, mRoundTimerClockInsideSpin });
    }

    al::setPaneStringFormat(this, "TxtRoundTimer", "%02i:%02i", mInfo->mRoundTimer.mMinutes, mInfo->mRoundTimer.mSeconds);
}

void CoinRunnerIcon::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

namespace {
    NERVE_IMPL(CoinRunnerIcon, Appear)
    NERVE_IMPL(CoinRunnerIcon, Wait)
    NERVE_IMPL(CoinRunnerIcon, End)
}
