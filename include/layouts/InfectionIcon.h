#pragma once

#include "al/layout/LayoutActor.h"
#include "al/layout/LayoutInitInfo.h"
#include "al/util/NerveUtil.h"

#include "logger.hpp"

// TODO: kill layout if going through loading zone or paused

class InfectionIcon : public al::LayoutActor {
    public:
        InfectionIcon(const char* name, const al::LayoutInitInfo& initInfo);

        void appear() override;

        bool tryStart();
        bool tryEnd();

        void showHiding();
        void showSeeking();
        
        void exeAppear();
        void exeWait();
        void exeEnd();

    private:
        struct InfectionInfo *mInfo;
};

namespace {
    NERVE_HEADER(InfectionIcon, Appear)
    NERVE_HEADER(InfectionIcon, Wait)
    NERVE_HEADER(InfectionIcon, End)
}