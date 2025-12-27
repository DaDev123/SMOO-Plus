#pragma once

#include "al/Library/Layout/LayoutActor.h"
#include "al/Library/Layout/LayoutInitInfo.h"
#include "al/Library/Nerve/NerveSetupUtil.h"

struct HideAndSeekInfo;
struct SardineInfo;
class StageScene;

enum class GameModePlayerSlotMode { HideAndSeek, Sardine };

class GameModePlayerSlot : public al::LayoutActor {
public:
    GameModePlayerSlot(const char* name, const al::LayoutInitInfo& initInfo, GameModePlayerSlotMode mode);

    void init(int index);
    void setScene(StageScene* scene) { mScene = scene; }
    void appear();
    bool tryEnd();
    bool tryStart();

    void exeAppear();
    void exeWait();
    void exeEnd();

    void showSlot();
    void hideSlot();

    void setSlotName(const char* name);

private:
    union {
        HideAndSeekInfo* mHnSInfo;
        SardineInfo* mSardineInfo;
    };
    GameModePlayerSlotMode mMode;
    StageScene* mScene = nullptr;
    int mPlayerIndex = 0;
    bool mIsVisible = false;
    bool mIsPlayer = false;
    float mIconRotation = 0.f;
};

namespace {
NERVE_IMPL(GameModePlayerSlot, Appear)
NERVE_IMPL(GameModePlayerSlot, Wait)
NERVE_IMPL(GameModePlayerSlot, End)

NERVES_MAKE_STRUCT(GameModePlayerSlot, Appear, Wait, End)
}  // namespace