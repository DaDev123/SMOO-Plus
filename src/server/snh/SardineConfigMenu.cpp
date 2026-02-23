#include "server/snh/SardineConfigMenu.hpp"

#include "logger.hpp"
#include "Scene/StageSceneStateServerConfig.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/snh/SardineMode.hpp"

SardineConfigMenu::SardineConfigMenu() : GameModeConfigMenu() {}

const sead::WFixedSafeString<0x200>* SardineConfigMenu::getStringData() {
    SardineInfo* curMode = GameModeManager::instance()->getInfo<SardineInfo>();

    // Update the persistent array
    mItems[0].copy(u"Sardine Gravity");
    mItems[1].copy(u"Sardine Tether");
    mItems[2].copy(u"Tether Snapping");

    return mItems.mBuffer;
}

void SardineConfigMenu::initMenu() {
    StageSceneStateServerConfig::setMenuItemCheck(mList->mListPartsArr[1]);
    StageSceneStateServerConfig::setMenuItemCheck(mList->mListPartsArr[2]);
    StageSceneStateServerConfig::setMenuItemCheck(mList->mListPartsArr[3]);
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
        al::startAction(mList->mListPartsArr[1], curMode->mIsUseGravity ? "On" : "Off", "State");
        return GameModeConfigMenu::UpdateAction::REFRESH;
    }
    case 1: {
        curMode->mIsTether = !curMode->mIsTether;
        Logger::log("Sardine Tether is now: %s\n", curMode->mIsTether ? "ON" : "OFF");
        al::startAction(mList->mListPartsArr[2], curMode->mIsTether ? "On" : "Off", "State");
        return GameModeConfigMenu::UpdateAction::REFRESH;
    }
    case 2: {
        curMode->mIsTetherSnap = !curMode->mIsTetherSnap;
        Logger::log("Tether Snapping is now: %s\n", curMode->mIsTetherSnap ? "ON" : "OFF");
        al::startAction(mList->mListPartsArr[3], curMode->mIsTetherSnap ? "On" : "Off", "State");
        return GameModeConfigMenu::UpdateAction::REFRESH;
    }
    default:
        Logger::log("Failed to interpret Index!\n");
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
}