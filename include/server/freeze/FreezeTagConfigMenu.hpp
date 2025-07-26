#pragma once

#include "server/gamemode/GameModeConfigMenu.hpp"
#include "game/Layouts/CommonVerticalList.h"
#include "server/gamemode/GameModeBase.hpp"
#include "TwistsConfig.hpp"

class Keyboard; // Forward declaration

class FreezeTagConfigMenu : public GameModeConfigMenu {
public:
    FreezeTagConfigMenu();
    
    void initMenu(const al::LayoutInitInfo &initInfo) override;
    const sead::WFixedSafeString<0x200> *getStringData() override;
    bool updateMenu(int selectIndex) override;

    const int getMenuSize() override; // Remove inline implementation

private:
    static constexpr int mMaxItemCount = 3; // Maximum possible items
    sead::SafeArray<sead::WFixedSafeString<0x200>, mMaxItemCount>* mConfigOptions;
    
    Keyboard* mScoreKeyboard;
    Keyboard* mRoundKeyboard;
    
    void updateOptionsText();
    int getCurrentMenuSize(); // Helper method to get current menu size
};