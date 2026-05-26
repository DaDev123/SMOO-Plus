#pragma once

#include "hk/hook/Trampoline.h"

#include "al/Library/Base/StringUtil.h"
#include "al/Library/Layout/LayoutInitInfo.h"
#include "al/Library/Nerve/Nerve.h"
#include "al/Library/Nerve/NerveUtil.h"

#include "game/Sequence/HakoniwaSequence.h"

#include "speedboot/CustomBootNerve.hpp"
#include "speedboot/HakoniwaSequenceSpeedboot.hpp"
#include "speedboot/SpeedbootLoad.hpp"

namespace speedboot {
static CustomBootNerve nrvSpeedboot;

static HakoniwaSequenceSpeedboot* speedbootState = nullptr;

static HkTrampoline prepareSpeedBootHook = [](TrampolineStatic(), BootLayout* boot, al::LayoutInitInfo& initInfo) -> void {
    al::NerveExecutor* e;
    __asm("MOV %[result], X19" : [result] "=r"(e));  // Hacky but it works

    if (al::isEqualString(typeid(*e).name(), typeid(HakoniwaSequence).name())) {
        new SpeedbootLoad(((HakoniwaSequence*)e)->mResourceLoader, initInfo, ((HakoniwaSequence*)e));
    }

    orig(boot, initInfo);
};

static void hakoniwaSetNerveSetup(al::IUseNerve* useNerve, al::Nerve* nerve) {
    al::setNerve(useNerve, &nrvSpeedboot);
    auto* sequence = static_cast<HakoniwaSequence*>(useNerve);
    speedbootState = new HakoniwaSequenceSpeedboot(sequence);
    al::initNerveState(useNerve, speedbootState, &nrvSpeedboot, "Speedboot");
}
}  // namespace speedboot
