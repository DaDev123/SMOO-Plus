#include "server/hns/HideAndSeekConfigMenu.hpp"
#include "logger.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/hns/HideAndSeekMode.hpp"

HideAndSeekConfigMenu::HideAndSeekConfigMenu() : GameModeConfigMenu() {}

const sead::WFixedSafeString<0x200>* HideAndSeekConfigMenu::getStringData() {
    HideAndSeekInfo* curMode = GameModeManager::instance()->getInfo<HideAndSeekInfo>();

    // Update the persistent array instead of creating a new one
    if (curMode && curMode->mIsUseGravity) {
        mItems[0].copy(u"H&S Gravity (ON)");
    } else {
        mItems[0].copy(u"H&S Gravity (OFF)");
    }

    return mItems.mBuffer;
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
        }
        return GameModeConfigMenu::UpdateAction::REFRESH;
    }

    return GameModeConfigMenu::UpdateAction::NOOP;
}