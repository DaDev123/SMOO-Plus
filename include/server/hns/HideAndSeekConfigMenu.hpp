#pragma once

#include "sead/container/seadSafeArray.h"

#include "server/gamemode/GameModeConfigMenu.hpp"

class HideAndSeekConfigMenu : public GameModeConfigMenu {
public:
    HideAndSeekConfigMenu();

    const sead::WFixedSafeString<0x200>* getStringData() override;
    GameModeConfigMenu::UpdateAction updateMenu(int selectIndex) override;

    const int getMenuSize() override { return 1; }

private:
    static constexpr int mItemCount = 1;
    sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount> mItems;  // Changed from pointer
};