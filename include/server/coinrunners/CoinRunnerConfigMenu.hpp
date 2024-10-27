#pragma once

#include "Keyboard.hpp"
#include "sead/container/seadSafeArray.h"
#include "server/gamemode/GameModeConfigMenu.hpp"
#include "server/coinrunners/CoinRunnerInfo.h"

class CoinRunnerConfigMenu : public GameModeConfigMenu {
    public:
        CoinRunnerConfigMenu();

        const sead::WFixedSafeString<0x200>* getStringData() override;
        GameModeConfigMenu::UpdateAction updateMenu(int selectIndex) override;

        const int getMenuSize() override { return CoinRunnerInfo::mIsHostMode ? 8 : 7; }

    private:
        static constexpr int mItemCount = 8;
        sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>* mItems = nullptr;
        Keyboard* mScoreKeyboard;
        Keyboard* mRoundKeyboard;
};
