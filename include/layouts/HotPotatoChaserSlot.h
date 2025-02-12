#pragma once

#include "al/layout/LayoutActor.h"
#include "al/layout/LayoutInitInfo.h"
#include "al/util/LayoutUtil.h"
#include "al/util/NerveUtil.h"

#include "logger.hpp"

// TODO: kill layout if going through loading zone or paused

class HotPotatoChaserSlot : public al::LayoutActor {
public:
    HotPotatoChaserSlot(const char* name, const al::LayoutInitInfo& initInfo);
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
    bool mIsPlayer = false;

    float mFreezeIconSize = 0.f;
    float mFreezeIconSpin = 0.f;
    int mChaserIndex;

private:
    struct HotPotatoInfo* mInfo;
};

namespace {
NERVE_HEADER(HotPotatoChaserSlot, Appear)
NERVE_HEADER(HotPotatoChaserSlot, Wait)
NERVE_HEADER(HotPotatoChaserSlot, End)
}