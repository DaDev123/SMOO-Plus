#pragma once

#include "al/Library/Factory/Factory.h"
#include "server/freeze/FreezeTagConfigMenu.hpp"
#include "server/gamemode/GameModeConfigMenu.hpp"
#include "server/hns/HideAndSeekConfigMenu.hpp"
#include "server/snh/SardineConfigMenu.hpp"

typedef GameModeConfigMenu* (*createMenu)(const char* name);

template <class T>
GameModeConfigMenu* createGameModeConfigMenu(const char* name) {
    return new T();
};

__attribute((used)) constexpr al::NameToCreator<createMenu> menuTable[] = {
    {"HideAndSeek", &createGameModeConfigMenu<HideAndSeekConfigMenu>},
    {"Sardine", &createGameModeConfigMenu<SardineConfigMenu>},
    {"FreezeTag", &createGameModeConfigMenu<FreezeTagConfigMenu>},
};

class GameModeConfigMenuFactory : public al::Factory<createMenu> {
public:
    GameModeConfigMenuFactory(const char* fName) : al::Factory<createMenu>(fName, menuTable) {
        this->mFactoryName = fName;
        this->mFactoryEntries = menuTable;
        this->mNumFactoryEntries = sizeof(menuTable) / sizeof(menuTable[0]);
    };

    constexpr static const char* getMenuName(int idx);
    constexpr static int getMenuCount();
};

constexpr const char* GameModeConfigMenuFactory::getMenuName(int idx) {
    if (idx >= 0 && idx < sizeof(menuTable) / sizeof(menuTable[0]))
        return menuTable[idx].name;
    return nullptr;
}

constexpr int GameModeConfigMenuFactory::getMenuCount() {
    return sizeof(menuTable) / sizeof(menuTable[0]);
}