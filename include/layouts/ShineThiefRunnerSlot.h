#pragma once

#include "al/Library/Layout/LayoutActor.h"
#include "al/Library/Layout/LayoutInitInfo.h"
#include "al/Library/Layout/LayoutUtil.h"
#include "al/Library/Nerve/NerveSetupUtil.h"

#include "Library/Layout/LayoutActorUtil.h"

// TODO: kill layout if going through loading zone or paused

class ShineThiefRunnerSlot : public al::LayoutActor {
public:
    ShineThiefRunnerSlot(const char* name, const al::LayoutInitInfo& initInfo);
    void init(int index);

    void appear() override;

    bool tryStart();
    bool tryEnd();

    void showSlot();
    void hideSlot();

    void setCaughtAngle();
    void setIconRotation();
    void setSlotName(const char* name) { al::setPaneStringFormat(this, "TxtRunnerName", "%s", name); };
    void setSlotScore(int score) { al::setPaneStringFormat(this, "TxtRunnerScore", "%04u", score); };

    void exeAppear();
    void exeWait();
    void exeEnd();

    bool mIsVisible = false;
    bool mIsPlayer = false;

    float mCaughtIconSize = 0.f;
    float mCaughtIconSpin = 0.f;
    float mSlotScale = 0.0f;
    int mRunnerIndex;

private:
    struct ShineThiefInfo* mInfo;
    uint8_t mHolderTeam = 0;
};

namespace {
NERVE_IMPL(ShineThiefRunnerSlot, Appear)
NERVE_IMPL(ShineThiefRunnerSlot, Wait)
NERVE_IMPL(ShineThiefRunnerSlot, End)

NERVES_MAKE_STRUCT(ShineThiefRunnerSlot, Appear, Wait, End)
}  // namespace