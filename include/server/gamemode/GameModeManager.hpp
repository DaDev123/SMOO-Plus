#pragma once

#include <heap/seadDisposer.h>
#include <heap/seadHeap.h>
#include <container/seadSafeArray.h>
#include "al/util.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/modifiers/ModeModifierBase.hpp"

class GameModeManager {
    SEAD_SINGLETON_DISPOSER(GameModeManager)
    GameModeManager();
    ~GameModeManager();

public:
    void setMode(GameMode mode);
    void initScene(const GameModeInitInfo& info);
    void begin();
    void end();
    void update();
    void pause();
    void unpause();

    GameMode getGameMode() const { return mCurMode; }
    template<class T> T* getMode() const { return static_cast<T*>(mCurModeBase); }
    template<class T> T* getInfo() const { return static_cast<T*>(mModeInfo); }
    template<class T> T* tryGetOrCreateInfo(GameMode mode);
    void setInfo(GameModeInfoBase* info) { mModeInfo = info; }
    static bool tryReceivePuppetMsg(const al::SensorMsg* msg, al::HitSensor* source, al::HitSensor* target) {
        return instance()->mCurModeBase && instance()->isActive() && instance()->mCurModeBase->mIsUsePuppetSensor ? instance()->mCurModeBase->receiveMsg(msg, source, target) : false;
    }
    static bool tryReceiveCapMsg(const al::SensorMsg* msg, al::HitSensor* source, al::HitSensor* target) {
        return instance()->mCurModeBase && instance()->isActive() && instance()->mCurModeBase->mIsUseCapSensor ? instance()->mCurModeBase->receiveMsg(msg, source, target) : false;
    }
    // returns false if default attack behavior should be used instead 
    static bool tryAttackPuppetSensor(al::HitSensor* source, al::HitSensor* target) {
        return instance()->mCurModeBase && instance()->isActive() && instance()->mCurModeBase->mIsUsePuppetSensor ? instance()->mCurModeBase->attackSensor(source, target) : false;
    }
    static bool tryAttackCapSensor(al::HitSensor* source, al::HitSensor* target) {
        return instance()->mCurModeBase && instance()->isActive() && instance()->mCurModeBase->mIsUseCapSensor ? instance()->mCurModeBase->attackSensor(source, target) : false;
    }

    static void processModePacket(Packet *packet) {
        if(instance()->mCurModeBase) {
            instance()->mCurModeBase->processPacket(packet);
        }
    }

    static Packet *createModePacket() {
        if(instance()->mCurModeBase) {
            return instance()->mCurModeBase->createPacket();
        }
        return nullptr;
    }

    template<class T>
    T* createModeInfo();

    sead::Heap* getHeap() { return mHeap; }
    static sead::Heap* getSceneHeap() { return al::getSceneHeap(); }
    void toggleActive();
    void setActive(bool active) { mActive = active; }
    void setPaused(bool paused);
    bool isMode(GameMode mode) const { return mCurMode == mode; }
    bool isActive() const { return mActive; }
    bool isModeAndActive(GameMode mode) const { return isMode(mode) && isActive(); }
    bool isModeRequireUI() { return isActive() && !mCurModeBase->isUseNormalUI(); }
    bool isPaused() const { return mPaused; }
    bool wasSceneTrans() const { return mWasSceneTrans; }
private:
    sead::Heap* mHeap = nullptr;

    bool mActive = false;
    bool mPaused = false;
    bool mWasSceneTrans = false;
    bool mWasSetMode = false;
    bool mWasPaused = false;
    GameMode mCurMode = GameMode::NONE;
    GameModeBase* mCurModeBase = nullptr;
    GameModeInfoBase *mModeInfo = nullptr;
    GameModeInitInfo *mLastInitInfo = nullptr;
    ModeModifierBase *mCurModifier = nullptr;
};

template<class T>
T* GameModeManager::createModeInfo() {
    sead::ScopedCurrentHeapSetter heapSetter(mHeap);

    T* info = new T();
    mModeInfo = info;
    return info;
}

template<class T>
T* GameModeManager::tryGetOrCreateInfo(GameMode mode) {
    if (mModeInfo && mModeInfo->mMode == mode)
        return static_cast<T*>(mModeInfo);

    if (mModeInfo)
        delete mModeInfo;  // attempt to destory previous info before creating new one

    return createModeInfo<T>();
}
