
#include "al/layout/LayoutInitInfo.h"
#include "al/nerve/Nerve.h"
#include "al/nerve/NerveStateBase.h"
#include "al/util/ControllerUtil.h"
#include "al/util/NerveUtil.h"
#include "al/wipe/WipeHolder.h"
#include "game/GameData/GameDataFunction.h"
#include "game/HakoniwaSequence/HakoniwaSequence.h"
#include "game/WorldList/WorldResourceLoader.h"
#include "speedboot/SpeedbootLoad.hpp"

NERVE_HEADER(HakoniwaSequence, LoadStage);
NERVE_HEADER(HakoniwaSequence, LoadWorldResourceWithBoot);
NERVE_IMPL(HakoniwaSequence, LoadStage);
NERVE_IMPL(HakoniwaSequence, LoadWorldResourceWithBoot);
namespace speedboot {
class CustomBootNerve : public al::Nerve {
public:
    void execute(al::NerveKeeper* keeper) override
    {
        // auto* sequence = static_cast<HakoniwaSequence*>(keeper->mParent);
        if (al::updateNerveState(keeper->mParent)) {
            al::setNerve(keeper->mParent, &nrvHakoniwaSequenceLoadStage);
        }
    }
};

CustomBootNerve nrvSpeedboot;

al::LayoutInitInfo copiedInitInfo;

namespace {
    NERVE_HEADER(HakoniwaSequenceSpeedboot, InitThread)
    NERVE_HEADER(HakoniwaSequenceSpeedboot, LoadStage)
    NERVE_HEADER(HakoniwaSequenceSpeedboot, WipeToKill)
} // namespace

struct HakoniwaSequenceSpeedboot : public al::NerveStateBase {
    HakoniwaSequenceSpeedboot(HakoniwaSequence* sequence)
        : al::NerveStateBase("Speedboot")
        , mSequence(sequence)
    {
        initNerve(&nrvHakoniwaSequenceSpeedbootLoadStage, 0);
    }

    void exeInitThread()
    {
        if (al::isFirstStep(this)) {
            mSequence->mInitThread->start();
        }

        if (mSequence->mInitThread->isDone()) {
            al::setNerve(this, &nrvHakoniwaSequenceSpeedbootLoadStage);
        }
    }

    bool isDoneLoading() const
    {
        return mSequence->mResourceLoader->isEndLoadWorldResource() && mSequence->mInitThread->isDone();
    }

    void exeLoadStage()
    {
        if (al::isFirstStep(this)) {
            mSequence->mInitThread->start();
            const char* name = GameDataFunction::getNextStageName(this->mSequence->mGameDataHolder);
            if (name == nullptr)
                name = GameDataFunction::getMainStageName(this->mSequence->mGameDataHolder, 0);
            int scenario = GameDataFunction::calcNextScenarioNo(this->mSequence->mGameDataHolder);
            if (scenario == -1)
                scenario = 1;
            int world = this->mSequence->mGameDataHolder.mData->mWorldList->tryFindWorldIndexByStageName(name);
            if (world > -1) {
                // Logger::log("Requesting %d-%d\n", world, scenario);
                mSequence->mResourceLoader->requestLoadWorldHomeStageResource(world, scenario);
            }
        }
        // Logger::log("Percentage %.02f, %s\n", mSequence->mResourceLoader->calcLoadPercent(),
        //             mSequence->mResourceLoader->isEndLoadWorldResource() ? "true" : "false");

        if (isDoneLoading()) {
            al::setNerve(this, &nrvHakoniwaSequenceSpeedbootWipeToKill);
        }
    }

    void exeWipeToKill()
    {
        if (al::isFirstStep(this)) {
            mSequence->mWipeHolder->startClose("FadeWhite", -1);
        }

        if (mSequence->mWipeHolder->isCloseEnd())
            kill();
    }

private:
    HakoniwaSequence* mSequence;
};

namespace {
    NERVE_IMPL(HakoniwaSequenceSpeedboot, InitThread);
    NERVE_IMPL(HakoniwaSequenceSpeedboot, LoadStage);
    NERVE_IMPL(HakoniwaSequenceSpeedboot, WipeToKill);
} // namespace

HakoniwaSequenceSpeedboot* deezNutsState;

extern "C" void _ZN10BootLayoutC1ERKN2al14LayoutInitInfoE(BootLayout* layout,
    const al::LayoutInitInfo& layoutInitInfo);

void prepareLayoutInitInfo(BootLayout* layout, const al::LayoutInitInfo& layoutInitInfo)
{
    register HakoniwaSequence* sequence asm("x19");
    new SpeedbootLoad(sequence->mResourceLoader, layoutInitInfo);
    _ZN10BootLayoutC1ERKN2al14LayoutInitInfoE(layout, layoutInitInfo);
}

void hakoniwaSetNerveSetup(al::IUseNerve* useNerve, al::Nerve* nerve)
{
    al::setNerve(useNerve, &nrvSpeedboot);
    auto* sequence = static_cast<HakoniwaSequence*>(useNerve);
    deezNutsState = new HakoniwaSequenceSpeedboot(sequence);
    al::initNerveState(useNerve, deezNutsState, &nrvSpeedboot, "Speedboot");
}
} // namespace speedboot