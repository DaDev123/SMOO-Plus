#include "server/inf/InfectionConfigMenu.hpp"
#include <cmath>
#include "logger.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/inf/InfectionMode.hpp"
#include "server/Client.hpp"

InfectionConfigMenu::InfectionConfigMenu() : GameModeConfigMenu() {}

void InfectionConfigMenu::initMenu(const al::LayoutInitInfo &initInfo) {
    
}

const sead::WFixedSafeString<0x200> *InfectionConfigMenu::getStringData() {
    sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>* gamemodeConfigOptions =
        new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();

    gamemodeConfigOptions->mBuffer[0].copy(u"Toggle Infection Gravity On");
    gamemodeConfigOptions->mBuffer[1].copy(u"Toggle Infection Gravity Off");

    return gamemodeConfigOptions->mBuffer;
}

bool InfectionConfigMenu::updateMenu(int selectIndex) {

    InfectionInfo *curMode = GameModeManager::instance()->getInfo<InfectionInfo>();

    Logger::log("Setting Gravity Mode.\n");

    if (!curMode) {
        Logger::log("Unable to Load Mode info!\n");
        return true;   
    }
    
    switch (selectIndex) {
        case 0: {
            if (GameModeManager::instance()->isMode(GameMode::Infection)) {
                curMode->mIsUseGravity = true;
            }
            return true;
        }
        case 1: {
            if (GameModeManager::instance()->isMode(GameMode::Infection)) {
                curMode->mIsUseGravity = false;
            }
            return true;
        }
        default:
            Logger::log("Failed to interpret Index!\n");
            return false;
    }
    
}