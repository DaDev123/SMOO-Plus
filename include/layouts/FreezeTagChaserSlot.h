#pragma once

#include "al/Library/Layout/LayoutActor.h"
#include "al/Library/Layout/LayoutActorUtil.h"
#include "al/Library/Layout/LayoutInitInfo.h"
#include "al/Library/Nerve/NerveSetupUtil.h"

// TODO: kill layout if going through loading zone or paused

class FreezeTagChaserSlot : public al::LayoutActor {
public:
    FreezeTagChaserSlot(const char* name, const al::LayoutInitInfo& initInfo);
    void init(int index);

    void appear() override;

    bool tryStart();
    bool tryEnd();

    void showSlot();
    void hideSlot();

    void setSlotName(const char* name) {
        al::setPaneStringFormat(this, "TxtChaserName", "%s", name);
    };
    void setSlotScore(int score) {
        al::setPaneStringFormat(this, "TxtChaserScore", "%04u", score);
    };

    void exeAppear();
    void exeWait();
    void exeEnd();

    bool mIsVisible = false;
    bool mIsPlayer = false;

    float mFreezeIconSize = 0.f;
    float mFreezeIconSpin = 0.f;
    int mChaserIndex;

private:
    struct FreezeTagInfo* mInfo;
};

namespace {
NERVE_IMPL(FreezeTagChaserSlot, Appear)
NERVE_IMPL(FreezeTagChaserSlot, Wait)
NERVE_IMPL(FreezeTagChaserSlot, End)

NERVES_MAKE_STRUCT(FreezeTagChaserSlot, Appear, Wait, End)
}  // namespace