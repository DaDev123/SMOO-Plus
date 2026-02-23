#include "server/hns/HideAndSeekConfigMenu.hpp"

#include "Library/Layout/LayoutActionFunction.h"
#include "logger.hpp"
#include "Scene/StageSceneStateServerConfig.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/hns/HideAndSeekMode.hpp"

HideAndSeekConfigMenu::HideAndSeekConfigMenu() : GameModeConfigMenu() {}

const sead::WFixedSafeString<0x200>* HideAndSeekConfigMenu::getStringData() {
    HideAndSeekInfo* curMode = GameModeManager::instance()->getInfo<HideAndSeekInfo>();

    // Update the persistent array instead of creating a new one
    if (curMode) {
        mItems[0].copy(u"H&S Gravity");
    }

    return mItems.mBuffer;
}

void HideAndSeekConfigMenu::initMenu() {
    StageSceneStateServerConfig::setMenuItemCheck(mList->mListPartsArr[1]);
}

GameModeConfigMenu::UpdateAction HideAndSeekConfigMenu::updateMenu(int selectIndex) {
    HideAndSeekInfo* curMode = GameModeManager::instance()->getInfo<HideAndSeekInfo>();

    Logger::log("Toggling Gravity Mode.\n");

    if (!curMode) {
        Logger::log("Unable to Load Mode info!\n");
        return GameModeConfigMenu::UpdateAction::NOOP;
    }

    if (selectIndex == 0) {
        if (GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK)) {
            curMode->mIsUseGravity = !curMode->mIsUseGravity;
            Logger::log("Gravity is now: %s\n", curMode->mIsUseGravity ? "ON" : "OFF");
            al::startAction(mList->mListPartsArr[1], curMode->mIsUseGravity ? "On" : "Off", "State");
        }
        return GameModeConfigMenu::UpdateAction::REFRESH;
    }

    return GameModeConfigMenu::UpdateAction::NOOP;
}