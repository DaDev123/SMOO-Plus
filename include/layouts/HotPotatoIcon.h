#pragma once

#include "al/layout/LayoutActor.h"
#include "al/layout/LayoutInitInfo.h"
#include "al/util/NerveUtil.h"

#include "layouts/HotPotatoChaserSlot.h"
#include "layouts/HotPotatoRunnerSlot.h"

#include "container/seadPtrArray.h"
#include "logger.hpp"
#include "math/seadVector.h"

// TODO: kill layout if going through loading zone or paused

class HotPotatoIcon : public al::LayoutActor {
public:
    HotPotatoIcon(const char* name, const al::LayoutInitInfo& initInfo);

    void appear() override;

    void setSpectateString(const char* spec) { mSpectateName = spec; }
    void setFreezeOverlayHeight();
    void setSpectateOverlayHeight();
    void setRoundTimerOverlay();

    void showEndgameScreen()
    {
        mEndgameIsDisplay = true;
        mEndgameTextAngle = 0.f;
        mEndgameTextSize = 0.f;
    };

    void hideEndgameScreen() { mEndgameIsDisplay = false; };

    void queueScoreEvent(int eventValue, const char* eventDesc);

    bool tryStart();
    bool tryEnd();

    void exeAppear();
    void exeWait();
    void exeEnd();

private:
    struct HotPotatoInfo* mInfo;

    // Runner and chaser display info
    sead::PtrArray<HotPotatoRunnerSlot> mRunnerSlots;
    sead::PtrArray<HotPotatoChaserSlot> mChaserSlots;
    const int mMaxRunners = 9;
    const int mMaxChasers = 9;

    // Spectate and genera; infp
    bool mIsRunner = true;
    bool mIsOverlayShowing = false;
    const char* mSpectateName = nullptr;

    // Score event tracker
    bool mScoreEventIsQueued = false;
    int mScoreEventValue = 0;
    const char* mScoreEventDesc = nullptr;

    float mScoreEventTime = -1.f; // Every time a score event starts, this timer is set to zero, increase over time to control anim
    sead::Vector3f mScoreEventPos = sead::Vector3f::zero;
    float mScoreEventScale = 0.f;

    // UI positioning and angle calculations
    float mRunnerFreezeIconAngle = 0.f;

    float mFreezeOverlayHeight = 415.f;

    float mSpectateOverlayHeight = -400.f;

    float mRoundTimerClockInsideSpin = 0.f;
    float mRoundTimerHeight = 390.f;
    float mRoundTimerScale = 1.f;

    // Endgame popup
    bool mEndgameIsDisplay = false;
    float mEndgameTextSize = 0.f;
    float mEndgameTextAngle = 0.f;
};

namespace {
NERVE_HEADER(HotPotatoIcon, Appear)
NERVE_HEADER(HotPotatoIcon, Wait)
NERVE_HEADER(HotPotatoIcon, End)
}