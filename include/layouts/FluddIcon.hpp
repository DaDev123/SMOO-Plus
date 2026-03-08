#pragma once

#include "al/Library/Layout/LayoutActor.h"
#include "al/Library/Layout/LayoutInitInfo.h"
#include "al/Library/Nerve/NerveSetupUtil.h"

#include "container/seadPtrArray.h"
#include "math/seadVector.h"
#include "Scene/Twists/Fludd/FluddTwist.hpp"

class FluddIcon : public al::LayoutActor {
public:
    FluddIcon(const char* name, const al::LayoutInitInfo& initInfo);

    void appear() override;

    bool tryStart();
    bool tryEnd();

    void exeAppear();
    void exeWait();
    void exeEnd();

    void updateDisplay();

    static FluddIcon* sInstance;
};

namespace {
NERVE_IMPL(FluddIcon, Appear)
NERVE_IMPL(FluddIcon, Wait)
NERVE_IMPL(FluddIcon, End)

NERVES_MAKE_STRUCT(FluddIcon, Appear, Wait, End)
}  // namespace