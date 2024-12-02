#pragma once

#include "al/nerve/Nerve.h"
#include "al/nerve/NerveKeeper.h"
#include "al/util/NerveUtil.h"
#include "game/HakoniwaSequence/HakoniwaSequence.h"

NERVE_HEADER(HakoniwaSequence, LoadStage);
NERVE_HEADER(HakoniwaSequence, LoadWorldResourceWithBoot);
NERVE_IMPL(HakoniwaSequence, LoadStage);
NERVE_IMPL(HakoniwaSequence, LoadWorldResourceWithBoot);

namespace speedboot {
    class CustomBootNerve : public al::Nerve {
        public:
            void execute(al::NerveKeeper* keeper) override {
                if (al::updateNerveState(keeper->mParent)) {
                    al::setNerve(keeper->mParent, &nrvHakoniwaSequenceLoadStage);
                }
            }
    };
}
