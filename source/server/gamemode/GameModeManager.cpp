#include "server/gamemode/GameModeManager.hpp"
#include <cstring>
#include <heap/seadExpHeap.h>
#include <basis/seadNew.h>
#include <heap/seadHeapMgr.h>
#include "al/util.hpp"
#include "logger.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "server/gamemode/modifiers/ModeModifierBase.hpp"
#include "server/gamemode/modifiers/ModifierFactory.hpp"
#include "types.h"

SEAD_SINGLETON_DISPOSER_IMPL(GameModeManager)

GameModeManager::GameModeManager() {
    mHeap = sead::ExpHeap::create(0x50000, "GameModeHeap", al::getSequenceHeap(), 8,
                                    sead::Heap::HeapDirection::cHeapDirection_Reverse, false);
    setMode(GameMode::HIDEANDSEEK); // set default gamemode
}

void GameModeManager::processModePacket(Packet* _packet) {
    GameModeInf<u8>* packet = (GameModeInf<u8>*)_packet;
    GameMode theirGameMode  = packet->gameMode();

    PuppetInfo* other = Client::findPuppetInfo(packet->mUserID, false);

    auto curModeBase    = instance()->mCurModeBase;
    bool hasOurGameMode = curModeBase && (theirGameMode == curModeBase->getMode());

    // only process if it is for our game mode
    if (hasOurGameMode) {
        curModeBase->processPacket(packet);
    } else if (other) {
        other->isIt = false;
    }

    // has the game mode of the other client changed?
    if (other && other->gameMode != theirGameMode) {
        other->gameMode = theirGameMode;
            Client::sendGamemodePacket();
    }
}

void GameModeManager::begin() {
    if (mCurModeBase) {
        sead::ScopedCurrentHeapSetter heapSetter(mHeap);
        mCurModeBase->begin();
        Logger::log("Beginning Mode.\n");
    }
}

void GameModeManager::end() {
    if (mCurModeBase) {
        sead::ScopedCurrentHeapSetter heapSetter(mHeap);
        mCurModeBase->end();
        Logger::log("Ending Mode.\n");
    }
}

void GameModeManager::pause() {
    if (mCurModeBase) {
        sead::ScopedCurrentHeapSetter heapSetter(mHeap);
        mCurModeBase->pause();
        Logger::log("Pausing Mode.\n");
    }
}

void GameModeManager::unpause() {
    if (mCurModeBase) {
        sead::ScopedCurrentHeapSetter heapSetter(mHeap);
        mCurModeBase->unpause();
        Logger::log("Unpausing Mode.\n");
    }
}

void GameModeManager::toggleActive() {
    mActive = !mActive;
}

void GameModeManager::setPaused(bool paused) {
    mPaused = paused;
}

void GameModeManager::setMode(GameMode mode) {
    mCurMode = mode;

    mWasSetMode = true; // recreate in initScene
}

void GameModeManager::update() {
    if (!mCurModeBase) return;
    bool inScene = al::getSceneHeap() != nullptr;
    if ((mActive && inScene && !mCurModeBase->isModeActive() && !mPaused && !mWasPaused) || mWasSceneTrans) {
        begin();
        mWasPaused = false;
    }
    if ((!mActive || !inScene) && mCurModeBase->isModeActive()) end();
    mWasSceneTrans = false;

    if (mActive) {
        if (mPaused && mCurModeBase->isModeActive()) {
            pause();
            mWasPaused = true;
        } else if (!mPaused && !mCurModeBase->isModeActive()) {
            unpause();
        }
    }

    if (mCurModeBase && mCurModeBase->isModeActive()) {
        sead::ScopedCurrentHeapSetter heapSetter(mHeap);
        mCurModeBase->update();
    }
}

void GameModeManager::initScene(const GameModeInitInfo& info) {
    sead::ScopedCurrentHeapSetter heapSetter(mHeap);

    if (mCurModeBase != nullptr && mWasSetMode) {
        delete mCurModeBase;
        mCurModeBase = nullptr;
    }

    if (mLastInitInfo != nullptr) {
        delete mLastInitInfo;
    }

    if (mCurMode == GameMode::NONE) {
        mCurModeBase = nullptr;
        delete mModeInfo;
        mModeInfo = nullptr;
        return;
    }

    mLastInitInfo = new GameModeInitInfo(info);

    if (mWasSetMode) {
        GameModeFactory factory("GameModeFactory");
        const char* name = factory.getModeString(mCurMode);
        mCurModeBase = factory.getCreator(name)(name);
        mWasSetMode = false;
    }

    if (mCurModeBase) {
        sead::ScopedCurrentHeapSetter heapSetter(GameModeManager::getSceneHeap());
        mCurModeBase->init(*mLastInitInfo);
        if (mCurModeBase->isModeActive())
            mWasSceneTrans = true;
    }
}