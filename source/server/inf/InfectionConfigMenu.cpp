#include "server/inf/InfectionConfigMenu.hpp"
#include <cmath>
#include "logger.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/inf/InfectionInfo.hpp"
#include "server/Client.hpp"

InfectionConfigMenu::InfectionConfigMenu() : GameModeConfigMenu() {
    mItems = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    mItems->mBuffer[0].copy(u"Toggle H&S Gravity (OFF)"); // TBD
}

void InfectionConfigMenu::initMenu(const al::LayoutInitInfo &initInfo) {}

const sead::WFixedSafeString<0x200>* InfectionConfigMenu::getStringData() {
    InfectionInfo* hns = GameModeManager::instance()->getInfo<InfectionInfo>();
    bool isMode = hns != nullptr && GameModeManager::instance()->isMode(GameMode::INFECTION);

    mItems->mBuffer[0].copy(
        isMode && hns->mIsUseGravity
        ? u"Toggle H&S Gravity (ON) "
        : u"Toggle H&S Gravity (OFF)"
    );

    return mItems->mBuffer;
}

GameModeConfigMenu::UpdateAction InfectionConfigMenu::updateMenu(int selectIndex) {
    switch (selectIndex) {
        case 0: {
            InfectionInfo* hns = GameModeManager::instance()->getInfo<InfectionInfo>();
            if (!hns) {
                Logger::log("Unable to Load Mode info!\n");
                return UpdateAction::NOOP;
            }
            if (GameModeManager::instance()->isMode(GameMode::INFECTION)) {
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
