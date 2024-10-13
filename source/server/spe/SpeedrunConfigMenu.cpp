#include "server/spe/SpeedrunConfigMenu.hpp"
#include <cmath>
#include "logger.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/spe/SpeedrunMode.hpp"
#include "server/Client.hpp"
#include "server/hns/HideAndSeekMode.hpp"

SpeedrunConfigMenu::SpeedrunConfigMenu() : GameModeConfigMenu() {}

void SpeedrunConfigMenu::initMenu(const al::LayoutInitInfo &initInfo) {
    
}

const sead::WFixedSafeString<0x200> *SpeedrunConfigMenu::getStringData() {
    sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>* gamemodeConfigOptions =
        new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();

    gamemodeConfigOptions->mBuffer[0].copy(u"Toggle Speedrun Gravity On");
    gamemodeConfigOptions->mBuffer[1].copy(u"Toggle Speedrun Gravity Off");

    return gamemodeConfigOptions->mBuffer;
}

bool SpeedrunConfigMenu::updateMenu(int selectIndex) {

    SpeedrunInfo *curMode = GameModeManager::instance()->getInfo<SpeedrunInfo>();

    Logger::log("Setting Gravity Mode.\n");

    if (!curMode) {
        Logger::log("Unable to Load Mode info!\n");
        return true;   
    }
    
    switch (selectIndex) {
        case 0: {
            if (GameModeManager::instance()->isMode(GameMode::SPEEDRUN)) {
                curMode->mIsUseGravity = true;
            }
            return true;
        }
        case 1: {
            if (GameModeManager::instance()->isMode(GameMode::SPEEDRUN)) {
                curMode->mIsUseGravity = false;
            }
            return true;
        }
        default:
            Logger::log("Failed to interpret Index!\n");
            return false;
    }
    
}