#pragma once

#include "Library/Layout/LayoutActorUtil.h"
#include "al/Library/Layout/LayoutActor.h"
#include "al/Library/Layout/LayoutInitInfo.h"
#include "al/Library/Layout/LayoutUtil.h"
#include "al/Library/Nerve/NerveSetupUtil.h"

// TODO: kill layout if going through loading zone or paused

class FreezeTagRunnerSlot : public al::LayoutActor {
public:
    FreezeTagRunnerSlot(const char* name, const al::LayoutInitInfo& initInfo);
    void init(int index);

    void appear() override;

    bool tryStart();
    bool tryEnd();

    void showSlot();
    void hideSlot();

    void setFreezeAngle();
    void setSlotName(const char* name) {
        al::setPaneStringFormat(this, "TxtRunnerName", "%s", name);
    };
    void setSlotScore(int score) {
        al::setPaneStringFormat(this, "TxtRunnerScore", "%04u", score);
    };

    void exeAppear();
    void exeWait();
    void exeEnd();

    bool mIsVisible = false;
    bool mIsPlayer = false;

    float mFreezeIconSize = 0.f;
    float mFreezeIconSpin = 0.f;
    int mRunnerIndex;

private:
    struct FreezeTagInfo* mInfo;
};

namespace {
NERVE_IMPL(FreezeTagRunnerSlot, Appear)
NERVE_IMPL(FreezeTagRunnerSlot, Wait)
NERVE_IMPL(FreezeTagRunnerSlot, End)

NERVES_MAKE_STRUCT(FreezeTagRunnerSlot, Appear, Wait, End)
}  // namespace