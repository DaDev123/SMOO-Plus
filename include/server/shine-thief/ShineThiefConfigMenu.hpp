#pragma once

#include "sead/container/seadSafeArray.h"

#include "Keyboard.hpp"
#include "server/gamemode/GameModeConfigMenu.hpp"

// Forward declaration
struct ShineThiefInfo;

class ShineThiefConfigMenu : public GameModeConfigMenu, public sead::IDisposer {
public:
    ShineThiefConfigMenu();
    ~ShineThiefConfigMenu() override = default;

    const sead::WFixedSafeString<0x200>* getStringData() override;
    GameModeConfigMenu::UpdateAction updateMenu(int selectIndex) override;

    const int getMenuSize() override { return 4; }

private:
    static constexpr int mItemCount = 4;
    sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount> mItems;
    Keyboard* mScoreKeyboard = nullptr;
    Keyboard* mRoundKeyboard = nullptr;
};