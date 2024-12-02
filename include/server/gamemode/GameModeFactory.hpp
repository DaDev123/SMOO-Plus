#pragma once

#include "al/factory/Factory.h"
#include "server/gamemode/GameModeBase.hpp"
#include "server/hns/HideAndSeekMode.hpp"
#include "server/sardines/SardineMode.hpp"
#include "server/freeze-tag/FreezeTagMode.hpp"

typedef GameModeBase* (*createMode)(const char* name);

template <class T>
GameModeBase* createGameMode(const char* name) {
    return new T(name);
};

__attribute((used)) constexpr al::NameToCreator<createMode> modeTable[] = {
    { "Legacy",      nullptr                          },
    { "HideAndSeek", &createGameMode<HideAndSeekMode> },
    { "Sardines",    &createGameMode<SardineMode>     },
    { "FreezeTag",   &createGameMode<FreezeTagMode>   },

};

constexpr const char* modeNames[] = {
    "Legacy",
    "Hide & Seek",
    "Sardines",
    "Freeze-Tag",
};

class GameModeFactory : public al::Factory<createMode> {
    public:
        GameModeFactory(const char* fName) {
            this->factoryName = fName;
            this->actorTable = modeTable;
            this->factoryCount = sizeof(modeTable) / sizeof(modeTable[0]);
        };

        constexpr static const char* getModeString(GameMode mode);
        constexpr static const char* getModeName(GameMode mode);
        constexpr static const char* getModeName(int idx);
        constexpr static int getModeCount();
};

// TODO: possibly use shadows' crc32 hash algorithm for this
constexpr const char* GameModeFactory::getModeString(GameMode mode) {
    if (mode >= 0 && (size_t)mode < sizeof(modeTable) / sizeof(modeTable[0])) {
        return modeTable[mode].creatorName;
    }
    if (mode == GameMode::NONE) {
        return "None";
    }
    return "Unknown";
}

constexpr const char* GameModeFactory::getModeName(GameMode mode) {
    if (mode >= 0 && (size_t)mode < sizeof(modeNames) / sizeof(modeNames[0])) {
        return modeNames[mode];
    }
    if (mode == GameMode::NONE) {
        return "None";
    }
    return "Unknown";
}

constexpr const char* GameModeFactory::getModeName(int idx) {
    if (idx >= 0 && (size_t)idx < sizeof(modeNames) / sizeof(modeNames[0])) {
        return modeNames[idx];
    }
    if (idx == -1) {
        return "None";
    }
    return "Unknown";
}

constexpr int GameModeFactory::getModeCount() {
    return sizeof(modeTable) / sizeof(modeTable[0]);
}
