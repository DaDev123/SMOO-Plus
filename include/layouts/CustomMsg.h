#pragma once

#include "al/layout/LayoutActor.h"
#include "al/layout/LayoutInitInfo.h"
#include "al/util/NerveUtil.h"

#include "logger.hpp"

class CustomMsg : public al::LayoutActor {
    public:
        CustomMsg(const char* text, const char* name, const al::LayoutInitInfo& initInfo);

        void appear() override;

        bool tryStart();
        bool tryEnd();

        void showHiding();
        void showSeeking();
        
        // New method to set custom text
        void setCustomText(const char* text);
        
        void exeAppear();
        void exeWait();
        void exeEnd();
        bool isActive() const { return mIsActive; }

    private:
        bool mIsActive = false;
        bool mHasTxtIcon = false;
        bool mHasTxtRank = false;
        bool mHasTxtName = false;
        char mCustomText[256] = {0}; // Store custom text
        bool mHasCustomText = false; // Flag to indicate if custom text is set
};

namespace {
    NERVE_HEADER(CustomMsg, Appear)
    NERVE_HEADER(CustomMsg, Wait)
    NERVE_HEADER(CustomMsg, End)
}