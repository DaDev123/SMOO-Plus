#pragma once

#include "al/nerve/NerveStateBase.h"
#include "al/util/NerveUtil.h"

#include "game/GameData/GameDataFunction.h"
#include "game/HakoniwaSequence/HakoniwaSequence.h"

namespace speedboot {
    namespace {
        NERVE_HEADER(HakoniwaSequenceSpeedboot, InitThread)
        NERVE_HEADER(HakoniwaSequenceSpeedboot, LoadStage)
        NERVE_HEADER(HakoniwaSequenceSpeedboot, WipeToKill)
    }

    struct HakoniwaSequenceSpeedboot : public al::NerveStateBase {
        public:
            HakoniwaSequenceSpeedboot(HakoniwaSequence* sequence) : al::NerveStateBase("Speedboot"), mSequence(sequence) {
                initNerve(&nrvHakoniwaSequenceSpeedbootLoadStage, 0);
            }

            void exeInitThread() {
                if (al::isFirstStep(this)) {
                    mSequence->mInitThread->start();
                }

                if (mSequence->mInitThread->isDone()) {
                    al::setNerve(this, &nrvHakoniwaSequenceSpeedbootLoadStage);
                }
            }

            bool isDoneLoading() const {
                return mSequence->mResourceLoader->isEndLoadWorldResource()
                    && mSequence->mInitThread->isDone()
                ;
            }

            void exeLoadStage() {
                if (al::isFirstStep(this)) {
                    mSequence->mInitThread->start();
                    const char* name = GameDataFunction::getNextStageName(this->mSequence->mGameDataHolder);
                    if (name == nullptr) {
                        name = GameDataFunction::getMainStageName(this->mSequence->mGameDataHolder, 0);
                    }
                    int scenario = GameDataFunction::calcNextScenarioNo(this->mSequence->mGameDataHolder);
                    if (scenario == -1) {
                        scenario = 1;
                    }
                    int world = this->mSequence->mGameDataHolder.mData->mWorldList->tryFindWorldIndexByStageName(name);
                    if (world > -1) {
                        mSequence->mResourceLoader->requestLoadWorldHomeStageResource(world, scenario);
                    }
                }

                if (isDoneLoading()) {
                    al::setNerve(this, &nrvHakoniwaSequenceSpeedbootWipeToKill);
                }
            }

            void exeWipeToKill() {
                if (al::isFirstStep(this)) {
                    mSequence->mWipeHolder->startClose("FadeWhite", -1);
                }

                if (mSequence->mWipeHolder->isCloseEnd()) {
                    kill();
                }
            }

        private:
            HakoniwaSequence* mSequence;
    };

    namespace {
        NERVE_IMPL(HakoniwaSequenceSpeedboot, InitThread);
        NERVE_IMPL(HakoniwaSequenceSpeedboot, LoadStage);
        NERVE_IMPL(HakoniwaSequenceSpeedboot, WipeToKill);
    }
}
