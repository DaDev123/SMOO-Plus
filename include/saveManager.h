#pragma once

#include "Library/Thread/AsyncFunctorThread.h"

namespace sead {
class FrameHeap;
}
class GameConfigData;

constexpr const char* sModFolder = "sd:/SMOO-Plus";
constexpr const char* sSettingsPath = "sd:/SMOO-Plus/settings.byml";

class SaveManager {
public:
    SaveManager();

    void startThread(GameConfigData* config);
    void write();
    void read(GameConfigData* config);

    typedef void (SaveManager::*ThreadFunc)(void);

public:
    static SaveManager* sInstance;
    static SaveManager* instance() { return sInstance; }

    al::AsyncFunctorThread mThread;
    sead::FrameHeap* mHeap = nullptr;
    GameConfigData* mConfig = nullptr;
};