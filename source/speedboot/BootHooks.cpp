#include "al/layout/LayoutInitInfo.h"
#include "al/nerve/Nerve.h"
#include "al/nerve/NerveStateBase.h"
#include "al/util/NerveUtil.h"
#include "game/HakoniwaSequence/HakoniwaSequence.h"
#include "game/GameData/GameDataHolderAccessor.h"
#include "game/GameData/GameDataFunction.h"

#include "speedboot/SpeedbootLoad.hpp"
#include "speedboot/CustomBootNerve.hpp"
#include "speedboot/HakoniwaSequenceSpeedboot.hpp"

namespace speedboot {
    CustomBootNerve nrvSpeedboot;
    
    HakoniwaSequenceSpeedboot* speedbootState = nullptr;

    // Constructor
    HakoniwaSequenceSpeedboot::HakoniwaSequenceSpeedboot(HakoniwaSequence* sequence)
        : al::NerveStateBase("Speedboot"), mSequence(sequence) {
        initNerve(&nrvHakoniwaSequenceSpeedbootLoadStage, 0);
    }

    // Initialize loading thread
    void HakoniwaSequenceSpeedboot::exeInitThread() {
        if (al::isFirstStep(this)) {
            mSequence->mInitThread->start();
        }

        if (mSequence->mInitThread->isDone()) {
            al::setNerve(this, &nrvHakoniwaSequenceSpeedbootLoadStage);
        }
    }

    // Load stage resources
    void HakoniwaSequenceSpeedboot::exeLoadStage() {
        if (al::isFirstStep(this)) {
            mSequence->mInitThread->start();
            
            // Get stage name from game data - mGameDataHolder is an accessor (not a pointer)
            const char* stageName = GameDataFunction::getNextStageName(mSequence->mGameDataHolder);
            if (!stageName) {
                stageName = GameDataFunction::getMainStageName(mSequence->mGameDataHolder, 0);
            }

            // Calculate scenario number
            s32 scenario = GameDataFunction::calcNextScenarioNo(mSequence->mGameDataHolder);
            if (scenario == -1) {
                scenario = 1;
            }

            // Request world resources - use dot notation since mGameDataHolder is not a pointer
            s32 worldIndex = mSequence->mGameDataHolder.mData->mWorldList->tryFindWorldIndexByStageName(stageName);
            if (worldIndex > -1) {
                mSequence->mResourceLoader->requestLoadWorldHomeStageResource(worldIndex, scenario);
            }
        }

        if (isDoneLoading()) {
            al::setNerve(this, &nrvHakoniwaSequenceSpeedbootWipeToKill);
        }
    }

    // Fade to black and complete speedboot
    void HakoniwaSequenceSpeedboot::exeWipeToKill() {
        if (al::isFirstStep(this)) {
            mSequence->mWipeHolder->startClose("FadeBlack", -1);
        }

        if (mSequence->mWipeHolder->isCloseEnd()) {
            kill();
        }
    }

    // Check if loading is complete
    bool HakoniwaSequenceSpeedboot::isDoneLoading() const {
        return mSequence->mResourceLoader->isEndLoadWorldResource() 
            && mSequence->mInitThread->isDone();
    }

    extern "C" void _ZN10BootLayoutC1ERKN2al14LayoutInitInfoE(BootLayout* layout, 
                                                              const al::LayoutInitInfo& layoutInitInfo);

    void prepareLayoutInitInfo(BootLayout* layout, const al::LayoutInitInfo& layoutInitInfo) {
        register HakoniwaSequence* sequence asm("x19");
        
        // SpeedbootLoad constructor takes (resourceLoader, layoutInitInfo, sequence)
        new SpeedbootLoad(
            sequence->mResourceLoader,
            layoutInitInfo,
            sequence
        );
        
        _ZN10BootLayoutC1ERKN2al14LayoutInitInfoE(layout, layoutInitInfo);
    }

    void hakoniwaSetNerveSetup(al::IUseNerve* useNerve, al::Nerve* nerve) {
        al::setNerve(useNerve, &nrvSpeedboot);
        auto* sequence = static_cast<HakoniwaSequence*>(useNerve);
        speedbootState = new HakoniwaSequenceSpeedboot(sequence);
        al::initNerveState(useNerve, speedbootState, &nrvSpeedboot, "Speedboot");
    }
}

// Define nerve implementations OUTSIDE the namespace
NERVE_IMPL(speedboot::HakoniwaSequenceSpeedboot, InitThread)
NERVE_IMPL(speedboot::HakoniwaSequenceSpeedboot, LoadStage)
NERVE_IMPL(speedboot::HakoniwaSequenceSpeedboot, WipeToKill)