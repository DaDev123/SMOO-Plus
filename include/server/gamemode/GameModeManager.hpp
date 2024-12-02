#pragma once

#include <heap/seadDisposer.h>
#include <heap/seadHeap.h>
#include <container/seadSafeArray.h>
#include "al/util.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeInitInfo.hpp"
#include "server/gamemode/modifiers/ModeModifierBase.hpp"

class GameModeManager {
    SEAD_SINGLETON_DISPOSER(GameModeManager)
    GameModeManager();

public:
    void setMode(GameMode mode);
    void initScene(const GameModeInitInfo& info);
    void begin();
    void end();
    void update();
    void pause();
    void unpause();

    GameMode getGameMode() const { return mCurMode; }
    GameMode getNextGameMode() const { return mNextMode; }
    template<class T> T* getMode() const { return static_cast<T*>(mCurModeBase); }
    template<class T> T* getInfo() const { return static_cast<T*>(mModeInfo); }
    void setInfo(GameModeInfoBase* info) { mModeInfo = info; }

    static void processModePacket(Packet* packet);
    static Packet* createModePacket();

    template<class T> T* createModeInfo();

    sead::Heap* getHeap() { return mHeap; }
    static sead::Heap* getSceneHeap() { return al::getSceneHeap(); }
    void toggleActive();
    void setActive(bool active) { mActive = active; }
    void setPaused(bool paused);
    bool isMode(GameMode mode) const { return mCurMode == mode; }
    bool isActive() const { return mActive; }
    bool isModeAndActive(GameMode mode) const { return isMode(mode) && isActive(); }
    bool isModeRequireUI();
    bool isPaused() const { return mPaused; }
    bool wasSceneTrans() const { return mWasSceneTrans; }

    static bool hasMarioCollision() { return instance()->mCurModeBase ? instance()->mCurModeBase->hasMarioCollision() : true;  }
    static bool hasMarioBounce()    { return instance()->mCurModeBase ? instance()->mCurModeBase->hasMarioBounce()    : true;  }
    static bool hasCappyCollision() { return instance()->mCurModeBase ? instance()->mCurModeBase->hasCappyCollision() : false; }
    static bool hasCappyBounce()    { return instance()->mCurModeBase ? instance()->mCurModeBase->hasCappyBounce()    : false; }

private:
    sead::Heap* mHeap = nullptr;

    bool mActive        = false;
    bool mPaused        = false;
    bool mWasSceneTrans = false;
    bool mWasSetMode    = false;
    bool mWasPaused     = false;

    GameMode          mCurMode      = GameMode::NONE;
    GameMode          mNextMode     = GameMode::NONE;
    GameModeBase*     mCurModeBase  = nullptr;
    GameModeInfoBase* mModeInfo     = nullptr;
    GameModeInitInfo* mLastInitInfo = nullptr;
    ModeModifierBase* mCurModifier  = nullptr;
};

// TODO: moving this method into the .cpp file crashes the game on stage enter
template<class T>
T* GameModeManager::createModeInfo() {
    sead::ScopedCurrentHeapSetter heapSetter(mHeap);

    T* info = new T();
    mModeInfo = info;
    return info;
}
