#pragma once

#include "sead/prim/seadSafeString.h"

#include "Layout/CommonVerticalList.h"
#include "Layout/SimpleLayoutMenu.h"

class GameModeConfigMenu {
public:
    enum UpdateAction {
        NOOP,
        CLOSE,
        REFRESH,
    };

    GameModeConfigMenu() = default;

    virtual UpdateAction updateMenu(int selectIndex) { return UpdateAction::NOOP; }

    virtual const sead::WFixedSafeString<0x200>* getStringData() { return nullptr; }

    virtual const int getMenuSize() { return 0; }
    virtual void initMenu() { return; };
    virtual void updateDataFromRollParts() { return; };

    SimpleLayoutMenu* mMenu;
    CommonVerticalList* mList;
};