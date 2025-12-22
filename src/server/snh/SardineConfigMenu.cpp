#include "server/snh/SardineConfigMenu.hpp"
#include "logger.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/snh/SardineMode.hpp"

SardineConfigMenu::SardineConfigMenu() : GameModeConfigMenu() {}

const sead::WFixedSafeString<0x200>* SardineConfigMenu::getStringData() {
    SardineInfo* curMode = GameModeManager::instance()->getInfo<SardineInfo>();

    // Update the persistent array
    if (curMode && curMode->mIsUseGravity) {
        mItems[0].copy(u"Sardine Gravity (ON)");
    } else {
        mItems[0].copy(u"Sardine Gravity (OFF)");
    }

    if (curMode && curMode->mIsTether) {
        mItems[1].copy(u"Sardine Tether (ON)");
    } else {
        mItems[1].copy(u"Sardine Tether (OFF)");
    }

    if (curMode && curMode->mIsTetherSnap) {
        mItems[2].copy(u"Tether Snapping (ON)");
    } else {
        mItems[2].copy(u"Tether Snapping (OFF)");
    }

    return mItems.mBuffer;
}

GameModeConfigMenu::UpdateAction SardineConfigMenu::updateMenu(int selectIndex) {
    SardineInfo* curMode = GameModeManager::instance()->getInfo<SardineInfo>();

    if (!curMode) {
        Logger::log("Unable to Load Mode info!\n");
        return GameModeConfigMenu::UpdateAction::NOOP;
    }

    if (!GameModeManager::instance()->isMode(GameMode::SARDINE)) {
        return GameModeConfigMenu::UpdateAction::NOOP;
    }

    switch (selectIndex) {
    case 0: {
        curMode->mIsUseGravity = !curMode->mIsUseGravity;
        Logger::log("Sardine Gravity is now: %s\n", curMode->mIsUseGravity ? "ON" : "OFF");
        return GameModeConfigMenu::UpdateAction::REFRESH;
    }
    case 1: {
        curMode->mIsTether = !curMode->mIsTether;
        Logger::log("Sardine Tether is now: %s\n", curMode->mIsTether ? "ON" : "OFF");
        return GameModeConfigMenu::UpdateAction::REFRESH;
    }
    case 2: {
        curMode->mIsTetherSnap = !curMode->mIsTetherSnap;
        Logger::log("Tether Snapping is now: %s\n", curMode->mIsTetherSnap ? "ON" : "OFF");
        return GameModeConfigMenu::UpdateAction::REFRESH;
    }
    default:
        Logger::log("Failed to interpret Index!\n");
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
}