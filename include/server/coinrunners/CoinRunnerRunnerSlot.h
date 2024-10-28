#pragma once

#include "al/util/LayoutUtil.h"
#include "al/util/NerveUtil.h"

class CoinRunnerInfo;

// TODO: kill layout if going through loading zone or paused

class CoinRunnerRunnerSlot : public al::LayoutActor {
    public:
        CoinRunnerRunnerSlot(const char* name, const al::LayoutInitInfo& initInfo);
        void init(int index);

        void appear() override;

        bool tryStart();
        bool tryEnd();

        void showSlot();
        void hideSlot();

        void setFreezeAngle();
        void setSlotName(const char* name) { al::setPaneStringFormat(this, "TxtRunnerName", "%s", name); };
        void setSlotScore(int score) { al::setPaneStringFormat(this, "TxtRunnerScore", "%04u", score); };

        void exeAppear();
        void exeWait();
        void exeEnd();

        bool mIsVisible = false;
        bool mIsPlayer  = false;

        float mFreezeIconSize = 0.f;
        float mFreezeIconSpin = 0.f;
        int mRunnerIndex;

    private:
        struct CoinRunnerInfo* mInfo;
};

namespace {
    NERVE_HEADER(CoinRunnerRunnerSlot, Appear)
    NERVE_HEADER(CoinRunnerRunnerSlot, Wait)
    NERVE_HEADER(CoinRunnerRunnerSlot, End)
}
