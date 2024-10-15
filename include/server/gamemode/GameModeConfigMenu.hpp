#pragma once

#include "game/Layouts/CommonVerticalList.h"

class GameModeConfigMenu {
public:
    enum UpdateAction {
        NOOP,
        CLOSE,
        REFRESH,
    };

    GameModeConfigMenu() = default;

    virtual void initMenu(const al::LayoutInitInfo &initInfo) {return;}

    virtual UpdateAction updateMenu(int selectIndex) {return UpdateAction::NOOP;}

    virtual const sead::WFixedSafeString<0x200>* getStringData() {return nullptr;}

    virtual const int getMenuSize() {return 0;}

};