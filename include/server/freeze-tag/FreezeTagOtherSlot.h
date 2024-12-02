#pragma once

#include "al/util/LayoutUtil.h"
#include "al/util/NerveUtil.h"

class FreezeTagInfo;

// TODO: kill layout if going through loading zone or paused

class FreezeTagOtherSlot : public al::LayoutActor {
    public:
        FreezeTagOtherSlot(const char* name, const al::LayoutInitInfo& initInfo);
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
        struct FreezeTagInfo* mInfo;
};

namespace {
    NERVE_HEADER(FreezeTagOtherSlot, Appear)
    NERVE_HEADER(FreezeTagOtherSlot, Wait)
    NERVE_HEADER(FreezeTagOtherSlot, End)
}
