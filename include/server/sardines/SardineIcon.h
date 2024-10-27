#pragma once

#include "al/layout/LayoutActor.h"
#include "al/layout/LayoutInitInfo.h"
#include "al/util/NerveUtil.h"

#include "server/sardines/SardineInfo.hpp"
#include "layouts/LayoutPlayerList.h"

// TODO: kill layout if going through loading zone or paused

class SardineIcon : public al::LayoutActor, LayoutPlayerList {
    public:
        SardineIcon(const char* name, const al::LayoutInitInfo& initInfo);

        void appear() override;

        bool tryStart();
        bool tryEnd();

        void showSolo();
        void showPack();

        void exeAppear();
        void exeWait();
        void exeEnd();

    protected:
        const char* getRoleIcon(bool isIt) override;
        GameMode getGameMode() override;
        bool isMeIt() override;

    private:
        struct SardineInfo* mInfo;
};

namespace {
    NERVE_HEADER(SardineIcon, Appear)
    NERVE_HEADER(SardineIcon, Wait)
    NERVE_HEADER(SardineIcon, End)
}
