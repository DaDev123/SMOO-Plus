#pragma once

#include "al/layout/LayoutActor.h"
#include "al/layout/LayoutInitInfo.h"
#include "al/util/NerveUtil.h"
#include "container/seadPtrArray.h"

#include "logger.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "layouts/GameModePlayerSlot.h"

// TODO: kill layout if going through loading zone or paused

class HideAndSeekIcon : public al::LayoutActor {
public:
    HideAndSeekIcon(const char* name, const al::LayoutInitInfo& initInfo);

    void appear() override;

    bool tryStart();
    bool tryEnd();

    void showHiding();
    void showSeeking();
    
    void exeAppear();
    void exeWait();
    void exeEnd();

void setCurScene(StageScene* scene) { 
        mCurScene = scene;
        // Update all player slots with the scene
        for (int i = 0; i < mMaxPlayers; i++) {
            mPlayerSlots.at(i)->setScene(scene);
        }
    }

private:
    struct HideAndSeekInfo *mInfo;
    sead::PtrArray<GameModePlayerSlot> mPlayerSlots;
    static constexpr int mMaxPlayers = 16;
    StageScene* mCurScene = nullptr;
};

namespace {
    NERVE_HEADER(HideAndSeekIcon, Appear)
    NERVE_HEADER(HideAndSeekIcon, Wait)
    NERVE_HEADER(HideAndSeekIcon, End)
}