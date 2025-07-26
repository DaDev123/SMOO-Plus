#pragma once

#include "al/layout/LayoutActor.h"
#include "al/layout/LayoutInitInfo.h"
#include "al/util/NerveUtil.h"

#include "logger.hpp"

class CustomMsg : public al::LayoutActor {
    public:
        CustomMsg(const char* name, const al::LayoutInitInfo& initInfo);

        void appear() override;

        bool tryStart();
        bool tryEnd();

        void showHiding();
        void showSeeking();
        
        void exeAppear();
        void exeWait();
        void exeEnd();
        bool isActive() const { return mIsActive; }

    private:
        bool mIsActive = false;
        bool mHasTxtIcon = false;
        bool mHasTxtRank = false;
        bool mHasTxtName = false;
};

namespace {
    NERVE_HEADER(CustomMsg, Appear)
    NERVE_HEADER(CustomMsg, Wait)
    NERVE_HEADER(CustomMsg, End)
}