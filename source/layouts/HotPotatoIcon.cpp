#include "layouts/HotPotatoIcon.h"
#include "al/string/StringTmp.h"
#include "al/util.hpp"
#include "al/util/MathUtil.h"
#include "layouts/HotPotatoRunnerSlot.h"
#include "logger.hpp"
#include "main.hpp"
#include "math/seadVector.h"
#include "prim/seadSafeString.h"
#include "puppets/PuppetInfo.h"
#include "rs/util.hpp"
#include "server/Client.hpp"
#include "server/hotpotato/HotPotatoMode.hpp"
#include <cstdio>
#include <cstring>

HotPotatoIcon::HotPotatoIcon(const char* name, const al::LayoutInitInfo& initInfo)
    : al::LayoutActor(name)
{
    al::initLayoutActor(this, initInfo, "HotPotatoIcon", 0);
    al::hidePane(this, "Endgame");

    mInfo = GameModeManager::instance()->getInfo<HotPotatoInfo>();
    mIsRunner = mInfo->mIsPlayerRunner;

    mRunnerSlots.tryAllocBuffer(mMaxRunners, al::getSceneHeap());
    for (int i = 0; i < mMaxRunners; i++) {
        HotPotatoRunnerSlot* newSlot = new (al::getSceneHeap()) HotPotatoRunnerSlot("RunnerSlot", initInfo);
        newSlot->init(i);
        mRunnerSlots.pushBack(newSlot);
    }

    mChaserSlots.tryAllocBuffer(mMaxChasers, al::getSceneHeap());
    for (int i = 0; i < mMaxChasers; i++) {
        HotPotatoChaserSlot* newSlot = new (al::getSceneHeap()) HotPotatoChaserSlot("ChaserSlot", initInfo);
        newSlot->init(i);
        mChaserSlots.pushBack(newSlot);
    }

    mSpectateName = nullptr;

    initNerve(&nrvHotPotatoIconEnd, 0);
    kill();
}

void HotPotatoIcon::appear()
{
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &nrvHotPotatoIconAppear);

    for (int i = 0; i < mMaxRunners; i++)
        mRunnerSlots.at(i)->tryStart();

    for (int i = 0; i < mMaxChasers; i++)
        mChaserSlots.at(i)->tryStart();

    al::LayoutActor::appear();
}

bool HotPotatoIcon::tryEnd()
{
    if (!al::isNerve(this, &nrvHotPotatoIconEnd)) {
        al::setNerve(this, &nrvHotPotatoIconEnd);
        for (int i = 0; i < mMaxRunners; i++)
            mRunnerSlots.at(i)->tryEnd();

        for (int i = 0; i < mMaxChasers; i++)
            mChaserSlots.at(i)->tryEnd();

        return true;
    }
    return false;
}

bool HotPotatoIcon::tryStart()
{
    if (!al::isNerve(this, &nrvHotPotatoIconWait) && !al::isNerve(this, &nrvHotPotatoIconAppear)) {
        appear();
        return true;
    }

    return false;
}

void HotPotatoIcon::exeAppear()
{
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvHotPotatoIconWait);
    }
}

void HotPotatoIcon::exeWait()
{
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

        if (mScoreEventTime > 3.75f)
            mScoreEventValue = 0;

        // Calculate score event pane's position
        sead::Vector3f targetPos = mScoreEventTime < 3.f ? sead::Vector3f(0.f, 235.f, 0.f) : sead::Vector3f(-650.f, 420.f, 0.f);
        if (!mInfo->mIsPlayerRunner)
            targetPos.x *= -1.f;

        al::lerpVec(&mScoreEventPos, mScoreEventPos, targetPos, 0.05f);
        al::setPaneLocalTrans(this, "ScoreEvent", mScoreEventPos);

        // Calculate score event pane's scale
        float targetScale = mScoreEventTime < 3.f ? 1.0f : 0.f;
        mScoreEventScale = al::lerpValue(mScoreEventScale, targetScale, 0.05f);
        al::setPaneLocalScale(this, "ScoreEvent", { mScoreEventScale, mScoreEventScale });
    }

    // Spectate UI
    if (mInfo->mIsPlayerFreeze && mSpectateName)
        al::setPaneStringFormat(this, "TxtSpectateTarget", "%s", mSpectateName);

    // Endgame UI
    if (mEndgameIsDisplay) {
        if (al::isHidePane(this, "Endgame"))
            al::showPane(this, "Endgame");

        mEndgameTextSize = al::lerpValue(mEndgameTextSize, 1.1f, 0.02f);
        mEndgameTextAngle = al::lerpValue(mEndgameTextAngle, 5.f, 0.01f);

        al::setPaneLocalScale(this, "PicEndgameText", { mEndgameTextSize, mEndgameTextSize });
        al::setPaneLocalRotate(this, "PicEndgameText", { 0.f, 0.f, mEndgameTextAngle });
    }

    if (!mEndgameIsDisplay && !al::isHidePane(this, "Endgame"))
        al::hidePane(this, "Endgame");
}

void HotPotatoIcon::queueScoreEvent(int eventValue, const char* eventDesc)
{
    mScoreEventTime = 0.f;
    mScoreEventIsQueued = true;
    mScoreEventValue += eventValue;
    mScoreEventValue = al::clamp(mScoreEventValue, 0, 99);

    mScoreEventDesc = eventDesc;
    mScoreEventPos = sead::Vector3f(0.f, 245.f, 0.f);
    mScoreEventScale = 1.35f;
}

void HotPotatoIcon::setFreezeOverlayHeight()
{
    // Show or hide the frozen UI overlay
    float targetHeight = mInfo->mIsPlayerFreeze ? 360.f : 415.f;
    mFreezeOverlayHeight = al::lerpValue(mFreezeOverlayHeight, targetHeight, 0.08f);
    al::setPaneLocalTrans(this, "PicFreezeOverlayTop", { 0.f, mFreezeOverlayHeight + 15.f, 0.f });
    al::setPaneLocalTrans(this, "PicFreezeOverlayBot", { 0.f, -mFreezeOverlayHeight, 0.f });
}

void HotPotatoIcon::setSpectateOverlayHeight()
{
    // Show or hide the spectator UI
    float targetHeight = mInfo->mIsPlayerFreeze && mInfo->mRunnerPlayers.size() > 0 && !mEndgameIsDisplay ? -250.f : -400.f;
    mSpectateOverlayHeight = al::lerpValue(mSpectateOverlayHeight, targetHeight, 0.04f);
    al::setPaneLocalTrans(this, "Spectate", { 0.f, mSpectateOverlayHeight, 0.f });
}

void HotPotatoIcon::setRoundTimerOverlay()
{
    // If round is active, set the timer's height on screen
    float targetHeight = mInfo->mIsRound && !mEndgameIsDisplay ? 330.f : 390.f;
    mRoundTimerHeight = al::lerpValue(mRoundTimerHeight, targetHeight, 0.03f);
    al::setPaneLocalTrans(this, "RoundTimer", { 0.f, mRoundTimerHeight, 0.f });

    // If time remaining is less than one minute, scale up larget
    float targetScale = mInfo->mIsRound && !mEndgameIsDisplay && mInfo->mRoundTimer.mMinutes <= 0 ? 1.66f : 1.f;
    mRoundTimerScale = al::lerpValue(mRoundTimerScale, targetScale, 0.02f);
    al::setPaneLocalScale(this, "RoundTimer", { mRoundTimerScale, mRoundTimerScale });

    // Spin the inside of the clock if a round is going
    if (mInfo->mIsRound) {
        mRoundTimerClockInsideSpin -= 1.2f;
        if (mRoundTimerClockInsideSpin < -360.f)
            mRoundTimerClockInsideSpin += 360.f;

        al::setPaneLocalRotate(this, "PicRoundTimerSpin", { 0.f, 0.f, mRoundTimerClockInsideSpin });
    }

    al::setPaneStringFormat(this, "TxtRoundTimer", "%02i:%02i", mInfo->mRoundTimer.mMinutes, mInfo->mRoundTimer.mSeconds);
}

void HotPotatoIcon::exeEnd()
{

    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

namespace {
NERVE_IMPL(HotPotatoIcon, Appear)
NERVE_IMPL(HotPotatoIcon, Wait)
NERVE_IMPL(HotPotatoIcon, End)
}