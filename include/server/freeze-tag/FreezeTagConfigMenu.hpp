#pragma once

#include "Keyboard.hpp"
#include "al/layout/LayoutInitInfo.h"
#include "container/seadSafeArray.h"
#include "prim/seadSafeString.h"
#include "server/gamemode/GameModeConfigMenu.hpp"

class FreezeTagConfigMenu : public GameModeConfigMenu {
    public:
        FreezeTagConfigMenu();

        void initMenu(const al::LayoutInitInfo& initInfo) override;
        const sead::WFixedSafeString<0x200>* getStringData() override;
        GameModeConfigMenu::UpdateAction updateMenu(int selectIndex) override;

        const int getMenuSize() override { return mItemCount; }

    private:
        static constexpr int mItemCount = 3;
        sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>* mItems = nullptr;
        Keyboard* mScoreKeyboard;
        Keyboard* mRoundKeyboard;
};
