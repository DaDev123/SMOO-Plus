#pragma once

#include "al/layout/LayoutActor.h"
#include "al/layout/LayoutInitInfo.h"
#include "al/util/NerveUtil.h"

#include "logger.hpp"
#include "server/gamemode/GameModeTimer.hpp"

// TODO: kill layout if going through loading zone or paused

class SpeedrunIcon : public al::LayoutActor {
    public:
        SpeedrunIcon(const char* name, const al::LayoutInitInfo& initInfo);

        void appear() override;

        bool tryStart();
        bool tryEnd();

        void showHiding();
        void showSeeking();
        
        void exeAppear();
        void exeWait();
        void exeEnd();

    private:
        struct SpeedrunInfo *mInfo;
};

namespace {
    NERVE_HEADER(SpeedrunIcon, Appear)
    NERVE_HEADER(SpeedrunIcon, Wait)
    NERVE_HEADER(SpeedrunIcon, End)
}