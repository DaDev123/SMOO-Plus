#pragma once

#include "sead/container/seadSafeArray.h"
#include "server/gamemode/GameModeConfigMenu.hpp"
#include "server/sardines/SardineInfo.hpp"

class SardineConfigMenu : public GameModeConfigMenu {
    public:
        SardineConfigMenu();

        const sead::WFixedSafeString<0x200>* getStringData() override;
        GameModeConfigMenu::UpdateAction updateMenu(int selectIndex) override;

        const int getMenuSize() override { return SardineInfo::mIsTether ? 7 : 6; }

    private:
        static constexpr int mItemCount = 7;
        sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>* mItems = nullptr;
};
