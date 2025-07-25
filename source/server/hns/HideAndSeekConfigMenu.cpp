#include "server/hns/HideAndSeekConfigMenu.hpp"
#include <cmath>
#include "logger.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/hns/HideAndSeekMode.hpp"
#include "server/Client.hpp"

HideAndSeekConfigMenu::HideAndSeekConfigMenu() : GameModeConfigMenu() {
    mConfigOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    updateOptionsText();
}

void HideAndSeekConfigMenu::initMenu(const al::LayoutInitInfo &initInfo) {
    
}

void HideAndSeekConfigMenu::updateOptionsText() {
    // Get current gravity state and display it in the toggle
    HideAndSeekInfo *curMode = GameModeManager::instance()->getInfo<HideAndSeekInfo>();
    bool isGravityEnabled = curMode ? curMode->mIsUseGravity : false;
    
    mConfigOptions->mBuffer[0].copy(
        isGravityEnabled ? u"Galaxy Gravity (ON) " : u"Galaxy Gravity (OFF)"
    );
}

const sead::WFixedSafeString<0x200> *HideAndSeekConfigMenu::getStringData() {
    // Update the text every time it's requested
    updateOptionsText();
    return mConfigOptions->mBuffer;
}

bool HideAndSeekConfigMenu::updateMenu(int selectIndex) {
    HideAndSeekInfo *curMode = GameModeManager::instance()->getInfo<HideAndSeekInfo>();

    if (!curMode) {
        Logger::log("Unable to Load Mode info!\n");
        return true;   
    }
    
    switch (selectIndex) {
        case 0: { // Toggle gravity
            if (GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK)) {
                curMode->mIsUseGravity = !curMode->mIsUseGravity;
                Logger::log("Toggled H&S Gravity to: %s\n", curMode->mIsUseGravity ? "ON" : "OFF");
            }
            return true; // Exit the menu after toggling
        }
        default:
            Logger::log("Failed to interpret Index!\n");
            return false;
    }
}