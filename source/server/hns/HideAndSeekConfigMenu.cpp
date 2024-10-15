#include "server/hns/HideAndSeekConfigMenu.hpp"
#include <cmath>
#include "logger.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/hns/HideAndSeekInfo.hpp"
#include "server/Client.hpp"

HideAndSeekConfigMenu::HideAndSeekConfigMenu() : GameModeConfigMenu() {
    mItems = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    mItems->mBuffer[0].copy(u"Toggle H&S Gravity (OFF)"); // TBD
}

void HideAndSeekConfigMenu::initMenu(const al::LayoutInitInfo &initInfo) {}

const sead::WFixedSafeString<0x200>* HideAndSeekConfigMenu::getStringData() {
    HideAndSeekInfo* hns = GameModeManager::instance()->getInfo<HideAndSeekInfo>();
    bool isMode = hns != nullptr && GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK);

    mItems->mBuffer[0].copy(
        isMode && hns->mIsUseGravity
        ? u"Toggle H&S Gravity (ON) "
        : u"Toggle H&S Gravity (OFF)"
    );

    return mItems->mBuffer;
}

GameModeConfigMenu::UpdateAction HideAndSeekConfigMenu::updateMenu(int selectIndex) {
    switch (selectIndex) {
        case 0: {
            HideAndSeekInfo* hns = GameModeManager::instance()->getInfo<HideAndSeekInfo>();
            if (!hns) {
                Logger::log("Unable to Load Mode info!\n");
                return UpdateAction::NOOP;
            }
            if (GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK)) {
                Logger::log("Setting Gravity Mode.\n");
                hns->mIsUseGravity = !hns->mIsUseGravity;
                return UpdateAction::REFRESH;
            }
            return UpdateAction::NOOP;
        }
        default: {
            Logger::log("Failed to interpret Index!\n");
            return UpdateAction::NOOP;
        }
    }
}
