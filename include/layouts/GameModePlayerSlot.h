#pragma once

#include "al/layout/LayoutActor.h"
#include "al/util/NerveUtil.h"

struct HideAndSeekInfo;
struct SardineInfo;

enum class GameModePlayerSlotMode {
    HideAndSeek,
    Sardine
};

class GameModePlayerSlot : public al::LayoutActor {
public:
    GameModePlayerSlot(const char* name, const al::LayoutInitInfo& initInfo, GameModePlayerSlotMode mode);
    
    void init(int index);
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
    int mPlayerIndex = 0;
    bool mIsVisible = false;
    bool mIsPlayer = false;
    float mIconRotation = 0.f;
};

namespace {
    NERVE_HEADER(GameModePlayerSlot, Appear)
    NERVE_HEADER(GameModePlayerSlot, Wait)
    NERVE_HEADER(GameModePlayerSlot, End)
}