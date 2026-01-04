#pragma once

#include "al/Library/Layout/LayoutActor.h"
#include "al/Library/Layout/LayoutInitInfo.h"
#include "al/Library/Nerve/NerveSetupUtil.h"

#include "container/seadPtrArray.h"
#include "layouts/GameModePlayerSlot.h"

// TODO: kill layout if going through loading zone or paused

class SpeedrunIcon : public al::LayoutActor {
public:
    SpeedrunIcon(const char* name, const al::LayoutInitInfo& initInfo);

    void appear() override;

    bool tryStart();
    bool tryEnd();

    void exeAppear();
    void exeWait();
    void exeEnd();
    void updateSpeedrunText();

    void setCurScene(StageScene* scene) {
        mCurScene = scene;
        // Update all player slots with the scene
        for (int i = 0; i < mMaxPlayers; i++) {
            mPlayerSlots.at(i)->setScene(scene);
        }
    }

    static SpeedrunIcon* sInstance;

private:
    sead::PtrArray<GameModePlayerSlot> mPlayerSlots;
    static constexpr int mMaxPlayers = 16;
    StageScene* mCurScene = nullptr;
};

namespace {
NERVE_IMPL(SpeedrunIcon, Appear)
NERVE_IMPL(SpeedrunIcon, Wait)
NERVE_IMPL(SpeedrunIcon, End)

NERVES_MAKE_STRUCT(SpeedrunIcon, Appear, Wait, End)
}  // namespace