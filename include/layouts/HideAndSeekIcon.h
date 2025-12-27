#pragma once

#include "al/Library/Layout/LayoutActor.h"
#include "al/Library/Layout/LayoutInitInfo.h"
#include "al/Library/Nerve/NerveSetupUtil.h"

#include "container/seadPtrArray.h"
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
    struct HideAndSeekInfo* mInfo;
    sead::PtrArray<GameModePlayerSlot> mPlayerSlots;
    static constexpr int mMaxPlayers = 16;
    StageScene* mCurScene = nullptr;
};

namespace {
NERVE_IMPL(HideAndSeekIcon, Appear)
NERVE_IMPL(HideAndSeekIcon, Wait)
NERVE_IMPL(HideAndSeekIcon, End)

NERVES_MAKE_STRUCT(HideAndSeekIcon, Appear, Wait, End)
}  // namespace