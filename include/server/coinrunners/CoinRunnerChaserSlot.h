#pragma once

#include "al/util/LayoutUtil.h"
#include "al/util/NerveUtil.h"

class CoinRunnerInfo;

// TODO: kill layout if going through loading zone or paused

class CoinRunnerChaserSlot : public al::LayoutActor {
    public:
        CoinRunnerChaserSlot(const char* name, const al::LayoutInitInfo& initInfo);
        void init(int index);

        void appear() override;

        bool tryStart();
        bool tryEnd();

        void showSlot();
        void hideSlot();

        void setSlotName(const char* name) { al::setPaneStringFormat(this, "TxtChaserName", "%s", name); };
        void setSlotScore(int score) { al::setPaneStringFormat(this, "TxtChaserScore", "%04u", score); };

        void exeAppear();
        void exeWait();
        void exeEnd();

        bool mIsVisible = false;
        bool mIsPlayer  = false;

        float mCoinIconSize = 0.f;
        float mCoinIconSpin = 0.f;
        int mChaserIndex;

    private:
        struct CoinRunnerInfo* mInfo;
};

namespace {
    NERVE_HEADER(CoinRunnerChaserSlot, Appear)
    NERVE_HEADER(CoinRunnerChaserSlot, Wait)
    NERVE_HEADER(CoinRunnerChaserSlot, End)
}
