#pragma once

#include "al/nerve/NerveStateBase.h"
#include "al/util/NerveUtil.h"

namespace speedboot {
    // Forward declare the nerve headers (declarations only in header)
    NERVE_HEADER(HakoniwaSequenceSpeedboot, InitThread)
    NERVE_HEADER(HakoniwaSequenceSpeedboot, LoadStage)
    NERVE_HEADER(HakoniwaSequenceSpeedboot, WipeToKill)

    class HakoniwaSequenceSpeedboot : public al::NerveStateBase {
    public:
        explicit HakoniwaSequenceSpeedboot(class HakoniwaSequence* sequence);

        // Nerve execution functions
        void exeInitThread();
        void exeLoadStage();
        void exeWipeToKill();

        /**
         * Check if both world resources and initialization thread are complete
         */
        bool isDoneLoading() const;

    private:
        class HakoniwaSequence* mSequence;
    };
}