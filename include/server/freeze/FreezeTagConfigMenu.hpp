#pragma once

#include "sead/container/seadSafeArray.h"

#include "Keyboard.hpp"
#include "server/gamemode/GameModeConfigMenu.hpp"

// Forward declaration
struct FreezeTagInfo;

class FreezeTagConfigMenu : public GameModeConfigMenu, public sead::IDisposer {
public:
    FreezeTagConfigMenu();
    ~FreezeTagConfigMenu() override = default;

    const sead::WFixedSafeString<0x200>* getStringData() override;
    GameModeConfigMenu::UpdateAction updateMenu(int selectIndex) override;

    const int getMenuSize() override { return 2; }  // Fixed size for now

private:
    static constexpr int mItemCount = 2;
    sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount> mItems;
    Keyboard* mScoreKeyboard = nullptr;
    Keyboard* mRoundKeyboard = nullptr;
};