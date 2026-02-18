#pragma once

#include "al/Library/Layout/LayoutActor.h"
#include "al/Library/Layout/LayoutActorUtil.h"
#include "al/Library/Layout/LayoutInitInfo.h"
#include "al/Library/Nerve/NerveSetupUtil.h"

class ShineThiefChaserSlot : public al::LayoutActor {
public:
    ShineThiefChaserSlot(const char* name, const al::LayoutInitInfo& initInfo);
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

    void updateNormalSlot();
    void updateTeamSlot();

    bool mIsVisible = false;
    bool mIsPlayer = false;

    float mShineThiefIconSize = 0.f;
    float mShineThiefIconSpin = 0.f;
    int mChaserIndex;

private:
    struct ShineThiefInfo* mInfo;
};

namespace {
NERVE_IMPL(ShineThiefChaserSlot, Appear)
NERVE_IMPL(ShineThiefChaserSlot, Wait)
NERVE_IMPL(ShineThiefChaserSlot, End)

NERVES_MAKE_STRUCT(ShineThiefChaserSlot, Appear, Wait, End)
}  // namespace