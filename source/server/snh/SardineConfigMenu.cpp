#include "server/snh/SardineConfigMenu.hpp"
#include "logger.hpp"
#include "server/Client.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/snh/SardineMode.hpp"
#include <cmath>

SardineConfigMenu::SardineConfigMenu() : GameModeConfigMenu() {
    mConfigOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    updateOptionsText();
}

void SardineConfigMenu::initMenu(const al::LayoutInitInfo& initInfo) {
}

void SardineConfigMenu::updateOptionsText() {
    SardineInfo* curMode = GameModeManager::instance()->getInfo<SardineInfo>();
    
    bool isGravityEnabled = curMode ? curMode->mIsUseGravity : false;
    bool isTetherEnabled = curMode ? curMode->mIsTether : false;
    bool isTetherSnapEnabled = curMode ? curMode->mIsTetherSnap : false;
    
    mConfigOptions->mBuffer[0].copy(
        isGravityEnabled ? u"Galaxy  Gravity (ON) " : u"Galaxy  Gravity (OFF)"
    );
    mConfigOptions->mBuffer[1].copy(
        isTetherEnabled ? u"Sardine Tether (ON) " : u"Sardine Tether (OFF)"
    );
    mConfigOptions->mBuffer[2].copy(
        isTetherSnapEnabled ? u"Tether Snapping (ON) " : u"Tether Snapping (OFF)"
    );
}

const sead::WFixedSafeString<0x200>* SardineConfigMenu::getStringData() {
    updateOptionsText();
    return mConfigOptions->mBuffer;
}

bool SardineConfigMenu::updateMenu(int selectIndex) {
    SardineInfo* curMode = GameModeManager::instance()->getInfo<SardineInfo>();

    if (!curMode) {
        Logger::log("Unable to Load Mode info!\n");
        return true;
    }

    switch (selectIndex) {
    case 0: { // Toggle gravity
        if (GameModeManager::instance()->isMode(GameMode::SARDINE)) {
            curMode->mIsUseGravity = !curMode->mIsUseGravity;
            Logger::log("Toggled Sardine Gravity to: %s\n", curMode->mIsUseGravity ? "ON" : "OFF");
        }
        return true;
    }
    case 1: { // Toggle tether
        if (GameModeManager::instance()->isMode(GameMode::SARDINE)) {
            curMode->mIsTether = !curMode->mIsTether;
            Logger::log("Toggled Sardine Tether to: %s\n", curMode->mIsTether ? "ON" : "OFF");
        }
        return true;
    }
    case 2: { // Toggle tether snapping
        if (GameModeManager::instance()->isMode(GameMode::SARDINE)) {
            curMode->mIsTetherSnap = !curMode->mIsTetherSnap;
            Logger::log("Toggled Tether Snapping to: %s\n", curMode->mIsTetherSnap ? "ON" : "OFF");
        }
        return true;
    }
    default:
        Logger::log("Failed to interpret Index!\n");
        return false;
    }
}