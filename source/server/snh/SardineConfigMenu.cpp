#include "server/snh/SardineConfigMenu.hpp"
#include <cmath>
#include "logger.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/snh/SardineMode.hpp"
#include "server/Client.hpp"

SardineConfigMenu::SardineConfigMenu() : GameModeConfigMenu() {}

void SardineConfigMenu::initMenu(const al::LayoutInitInfo &initInfo) {
    
}

const sead::WFixedSafeString<0x200> *SardineConfigMenu::getStringData() {
    sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>* gamemodeConfigOptions =
        new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();

    gamemodeConfigOptions->mBuffer[0].copy(u"Toggle Sardines Gravity On");
    gamemodeConfigOptions->mBuffer[1].copy(u"Toggle Sardines Off");

    return gamemodeConfigOptions->mBuffer;
}

bool SardineConfigMenu::updateMenu(int selectIndex) {

    SardineInfo *curMode = GameModeManager::instance()->getInfo<SardineInfo>();

    Logger::log("Setting Gravity Mode.\n");

    if (!curMode) {
        Logger::log("Unable to Load Mode info!\n");
        return true;   
    }
    
    switch (selectIndex) {
        case 0: {
            if (GameModeManager::instance()->isMode(GameMode::SARDINE)) {
                curMode->mIsUseGravity = true;
            }
            return true;
        }
        case 1: {
            if (GameModeManager::instance()->isMode(GameMode::SARDINE)) {
                curMode->mIsUseGravity = false;
            }
            return true;
        }
        default:
            Logger::log("Failed to interpret Index!\n");
            return false;
    }
    
}