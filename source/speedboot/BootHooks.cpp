
#include "al/layout/LayoutInitInfo.h"
#include "al/nerve/Nerve.h"
#include "al/util/NerveUtil.h"
#include "game/HakoniwaSequence/HakoniwaSequence.h"

#include "speedboot/SpeedbootLoad.hpp"
#include "speedboot/CustomBootNerve.hpp"
#include "speedboot/HakoniwaSequenceSpeedboot.hpp"

namespace speedboot {
    CustomBootNerve nrvSpeedboot;
    const bool speedbootAutoload = false; // set this to true, to automatically load the game, which skips the main menu (this has issues with empty save files)

    al::LayoutInitInfo copiedInitInfo;

    HakoniwaSequenceSpeedboot* deezNutsState;

    extern "C" void _ZN10BootLayoutC1ERKN2al14LayoutInitInfoE(BootLayout* layout, const al::LayoutInitInfo& layoutInitInfo);

    void prepareLayoutInitInfo(BootLayout* layout, const al::LayoutInitInfo& layoutInitInfo) {
        register HakoniwaSequence* sequence asm("x19");
        new SpeedbootLoad(
            sequence->mResourceLoader,
            layoutInitInfo,
            speedbootAutoload ? 0.f : 10.f
        );
        _ZN10BootLayoutC1ERKN2al14LayoutInitInfoE(layout, layoutInitInfo);
    }

    void hakoniwaSetNerveSetup(al::IUseNerve* useNerve, al::Nerve* nerve) {
        if (!speedbootAutoload) {
            return;
        }
        al::setNerve(useNerve, &nrvSpeedboot);
        auto* sequence = static_cast<HakoniwaSequence*>(useNerve);
        deezNutsState = new HakoniwaSequenceSpeedboot(sequence);
        al::initNerveState(useNerve, deezNutsState, &nrvSpeedboot, "Speedboot");
    }
}
