#pragma once

#include "al/util/LayoutUtil.h"
#include "al/util/NerveUtil.h"

class CoinRunnerInfo;

// TODO: kill layout if going through loading zone or paused

class CoinRunnerOtherSlot : public al::LayoutActor {
    public:
        CoinRunnerOtherSlot(const char* name, const al::LayoutInitInfo& initInfo);
        void init(int index);

        void appear() override;

        bool tryStart();
        bool tryEnd();

        void showSlot();
        void hideSlot();

        void setSlotName(const char* name) { al::setPaneStringFormat(this, "TxtOtherName", "%s", name); };
        void setSlotScore(int score) { al::setPaneStringFormat(this, "TxtOtherScore", "%04u", score); };

        void exeAppear();
        void exeWait();
        void exeEnd();

        bool mIsVisible = false;
        bool mIsPlayer  = false;

        float mFreezeIconSize = 0.f;
        float mFreezeIconSpin = 0.f;
        int mOtherIndex;

    private:
        struct CoinRunnerInfo* mInfo;
};

namespace {
    NERVE_HEADER(CoinRunnerOtherSlot, Appear)
    NERVE_HEADER(CoinRunnerOtherSlot, Wait)
    NERVE_HEADER(CoinRunnerOtherSlot, End)
}
