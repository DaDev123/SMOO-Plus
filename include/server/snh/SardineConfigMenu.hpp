#pragma once

#include "sead/container/seadSafeArray.h"

#include "server/gamemode/GameModeConfigMenu.hpp"

class SardineConfigMenu : public GameModeConfigMenu {
public:
    SardineConfigMenu();

    const sead::WFixedSafeString<0x200>* getStringData() override;
    GameModeConfigMenu::UpdateAction updateMenu(int selectIndex) override;

    const int getMenuSize() override { return 3; }
    void initMenu() override;

private:
    static constexpr int mItemCount = 3;
    sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount> mItems;  // Changed from pointer
};