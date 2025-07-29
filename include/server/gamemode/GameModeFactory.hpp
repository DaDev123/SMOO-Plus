#pragma once

#include "al/factory/Factory.h"
#include "server/gamemode/GameModeBase.hpp"
#include "server/hns/HideAndSeekMode.hpp"
#include "server/freeze/FreezeTagMode.hpp"
#include "server/snh/SardineMode.hpp"

typedef GameModeBase* (*createMode)(const char* name);

template<class T>
GameModeBase* createGameMode(const char* name) {
    return new T(name);
};

__attribute((used)) constexpr al::NameToCreator<createMode> modeTable[] = {
    {"HideAndSeek", &createGameMode<HideAndSeekMode>},
    {"Sardine", &createGameMode<SardineMode>},
    {"FreezeTag", &createGameMode<FreezeTagMode>}
};

constexpr const char* modeNames[] = {
    "None",          // Index 0: for NONE (-1)
    "Hide and Seek", // Index 1: for HIDENSEEK (0)
    "Sardines",      // Index 2: for SARDINE (1)
    "Freeze Tag"     // Index 3: for FREEZETAG (2)
};

class GameModeFactory : public al::Factory<createMode> {
    public:
        GameModeFactory(const char *fName) {
            this->factoryName = fName;
            this->actorTable = modeTable;
            this->factoryCount = sizeof(modeTable)/sizeof(modeTable[0]);
        };

        constexpr static const char* getModeString(GameMode mode);
        constexpr static const char* getModeName(GameMode mode);
        constexpr static const char* getModeName(int idx);
        constexpr static int getModeCount();
};

constexpr const char* GameModeFactory::getModeString(GameMode mode) {
    if(mode == GameMode::NONE)
        return "None";  // Special case since NONE isn't in modeTable
        
    if(mode >= 0 && (size_t)mode < sizeof(modeTable)/sizeof(modeTable[0]))
        return modeTable[mode].creatorName;
    return "Unknown";
}

constexpr const char* GameModeFactory::getModeName(GameMode mode) {
    int index = (int)mode + 1;  // -1 becomes 0, 0 becomes 1, etc.
        
    if(index >= 0 && (size_t)index < sizeof(modeNames)/sizeof(modeNames[0]))
        return modeNames[index];
    return "Unknown";
}

constexpr const char* GameModeFactory::getModeName(int idx) {
    if(idx == -1)
        return modeNames[0]; // "None"
    
    // For gamemode select screen: idx 0,1,2 should map to actual game modes
    // Skip the "None" entry and map directly to game modes
    int adjustedIdx = idx + 1; // Skip "None" at index 0
    if(adjustedIdx >= 1 && (size_t)adjustedIdx < sizeof(modeNames)/sizeof(modeNames[0]))
        return modeNames[adjustedIdx];
    return "Unknown";
}

constexpr int GameModeFactory::getModeCount() {
    return sizeof(modeTable)/sizeof(modeTable[0]);
}