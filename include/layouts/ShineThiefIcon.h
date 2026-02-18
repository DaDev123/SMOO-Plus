#pragma once

#include "al/Library/Layout/LayoutActor.h"
#include "al/Library/Layout/LayoutInitInfo.h"
#include "al/Library/Nerve/NerveSetupUtil.h"

#include "container/seadPtrArray.h"
#include "layouts/ShineThiefChaserSlot.h"
#include "layouts/ShineThiefRunnerSlot.h"

class ShineThiefIcon : public al::LayoutActor {
public:
    ShineThiefIcon(const char* name, const al::LayoutInitInfo& initInfo);

    void appear() override;

    void setSpectateString(const char* spec) { mSpectateName = spec; }
    void setCaughtOverlayHeight();
    void setSpectateOverlayHeight();
    void setRoundTimerOverlay();
    void updateTeamDisplay();
    void updateHolderBorder();

    void showEndgameScreen() {
        mEndgameIsDisplay = true;
        mEndgameTextAngle = 0.f;
        mEndgameTextSize = 0.f;
        mEndgameTextTimer = 0.0f;
        mEndgameTextScale = 0.75f;
    };
    void showShineRespawned();

    void hideEndgameScreen() { mEndgameIsDisplay = false; };

    void queueScoreEvent(int eventValue, const char* eventDesc);

    bool tryStart();
    bool tryEnd();

    void exeAppear();
    void exeWait();
    void exeEnd();

    void showCountdown(int number);
    void hideCountdown();
    void startCountdownScaleDown();
    void show45SecondWarning();

private:
    struct ShineThiefInfo* mInfo;

    // Runner and chaser display info
    sead::PtrArray<ShineThiefRunnerSlot> mRunnerSlots;
    sead::PtrArray<ShineThiefChaserSlot> mChaserSlots;
    const int mMaxRunners = 9;
    const int mMaxChasers = 9;

    // Spectate and general info
    bool mIsRunner = true;
    bool mIsOverlayShowing = false;
    const char* mSpectateName = nullptr;

    // Score event tracker
    bool mScoreEventIsQueued = false;
    int mScoreEventValue = 0;
    const char* mScoreEventDesc = nullptr;

    float mScoreEventTime = -1.f;
    sead::Vector3f mScoreEventPos = sead::Vector3f::zero;
    float mScoreEventScale = 0.f;

    // UI positioning and angle calculations
    float mRunnerCaughtIconAngle = 0.f;

    float mCaughtOverlayHeight = 415.f;

    float mSpectateOverlayHeight = -400.f;

    float mRoundTimerClockInsideSpin = 0.f;
    float mRoundTimerHeight = 390.f;
    float mRoundTimerScale = 1.f;

    float mCountdownScale = 0.0f;
    int mCountdownFrames = 0;
    bool mCountdownScalingDown = false;

    bool mEndgameIsDisplay = false;
    float mEndgameTextSize = 0.f;
    float mEndgameTextAngle = 0.f;
    float mEndgameTextScale = 0.0f;
    float mEndgameTextTimer = -1.0f;

    float mWarning45sScale = 0.0f;
    float mWarning45sTimer = -1.0f;
    float mShineRespawnScale = 0.0f;
    float mShineRespawnTimer = -1.0f;

    float mBorderPulseTimer = 0.0f;
};

namespace {
NERVE_IMPL(ShineThiefIcon, Appear)
NERVE_IMPL(ShineThiefIcon, Wait)
NERVE_IMPL(ShineThiefIcon, End)

NERVES_MAKE_STRUCT(ShineThiefIcon, Appear, Wait, End)
}  // namespace