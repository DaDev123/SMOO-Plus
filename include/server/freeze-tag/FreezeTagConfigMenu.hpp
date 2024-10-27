#pragma once

#include "Keyboard.hpp"
#include "sead/container/seadSafeArray.h"
#include "server/gamemode/GameModeConfigMenu.hpp"
#include "server/freeze-tag/FreezeTagInfo.h"

class FreezeTagConfigMenu : public GameModeConfigMenu {
    public:
        FreezeTagConfigMenu();

        const sead::WFixedSafeString<0x200>* getStringData() override;
        GameModeConfigMenu::UpdateAction updateMenu(int selectIndex) override;

        const int getMenuSize() override { return FreezeTagInfo::mIsHostMode ? 8 : 7; }

    private:
        static constexpr int mItemCount = 8;
        sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>* mItems = nullptr;
        Keyboard* mScoreKeyboard;
        Keyboard* mRoundKeyboard;
};
