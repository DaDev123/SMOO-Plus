#pragma once

#include "al/layout/LayoutActor.h"
#include "al/layout/LayoutInitInfo.h"
#include "al/util/NerveUtil.h"
#include "sead/container/seadPtrArray.h"
#include "sead/math/seadVector.h"

class CoinRunnerInfo;
class CoinRunnerRunnerSlot;
class CoinRunnerChaserSlot;
class CoinRunnerOtherSlot;

// TODO: kill layout if going through loading zone or paused

class CoinRunnerIcon : public al::LayoutActor {
    public:
        CoinRunnerIcon(const char* name, const al::LayoutInitInfo& initInfo);

        void appear() override;

        void setSpectateString(const char* spec) { mSpectateName = spec; }
        void setCoinOverlayHeight();
        void setSpectateOverlayHeight();
        void setRoundTimerOverlay();

        void showEndgameScreen() {
            mEndgameIsDisplay = true;
            mEndgameTextAngle = 0.f;
            mEndgameTextSize  = 0.f;
        };

        void hideEndgameScreen() { mEndgameIsDisplay = false; };

        void queueScoreEvent(int eventValue, const char* eventDesc);

        bool tryStart();
        bool tryEnd();

        void exeAppear();
        void exeWait();
        void exeEnd();

    private:
        struct CoinRunnerInfo* mInfo;

        // Runner and chaser display info
        sead::PtrArray<CoinRunnerRunnerSlot> mRunnerSlots;
        sead::PtrArray<CoinRunnerChaserSlot> mChaserSlots;
        sead::PtrArray<CoinRunnerOtherSlot>  mOtherSlots;
        const int mMaxRunners = 9;
        const int mMaxChasers = 9;
        const int mMaxOthers  = 9;

        // Spectate and general info
        bool        mIsRunner         = true;
        bool        mIsOverlayShowing = false;
        const char* mSpectateName     = nullptr;

        // Score event tracker
        bool        mScoreEventIsQueued = false;
        int         mScoreEventValue    = 0;
        const char* mScoreEventDesc     = nullptr;

        float          mScoreEventTime  = -1.f; // Every time a score event starts, this timer is set to zero, increase over time to control anim
        sead::Vector3f mScoreEventPos   = sead::Vector3f::zero;
        float          mScoreEventScale = 0.f;

        // UI positioning and angle calculations
        float mRunnerCoinIconAngle = 0.f;

        float mCoinOverlayHeight = 415.f;

        float mSpectateOverlayHeight = -400.f;

        float mRoundTimerClockInsideSpin = 0.f;
        float mRoundTimerHeight          = 390.f;
        float mRoundTimerScale           = 1.f;

        // Endgame popup
        bool  mEndgameIsDisplay = false;
        float mEndgameTextSize  = 0.f;
        float mEndgameTextAngle = 0.f;
};

namespace {
    NERVE_HEADER(CoinRunnerIcon, Appear)
    NERVE_HEADER(CoinRunnerIcon, Wait)
    NERVE_HEADER(CoinRunnerIcon, End)
}
