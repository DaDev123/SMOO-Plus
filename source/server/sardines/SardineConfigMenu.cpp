#include "server/sardines/SardineConfigMenu.hpp"
#include "logger.hpp"
#include "server/Client.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/sardines/SardineInfo.hpp"
#include <cmath>

SardineConfigMenu::SardineConfigMenu() : GameModeConfigMenu() {
    mItems = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    mItems->mBuffer[0].copy(u"Sardine Gravity (OFF)"); // TBD
    mItems->mBuffer[1].copy(u"Sardine Tether (OFF)");  // TBD
    mItems->mBuffer[2].copy(u"Tether Snapping (OFF)"); // TBD
}

void SardineConfigMenu::initMenu(const al::LayoutInitInfo& initInfo) {}

const sead::WFixedSafeString<0x200>* SardineConfigMenu::getStringData() {
    SardineInfo* sardine = GameModeManager::instance()->getInfo<SardineInfo>();
    bool isMode = sardine != nullptr && GameModeManager::instance()->isMode(GameMode::SARDINE);

    mItems->mBuffer[0].copy(
        isMode && sardine->mIsUseGravity
        ? u"Sardine Gravity (ON) "
        : u"Sardine Gravity (OFF)"
    );
    mItems->mBuffer[1].copy(
        isMode && sardine->mIsTether
        ? u"Sardine Tether (ON) "
        : u"Sardine Tether (OFF)"
    );
    mItems->mBuffer[2].copy(
        isMode && sardine->mIsTether && sardine->mIsTetherSnap
        ? u"Tether Snapping (ON) "
        : u"Tether Snapping (OFF)"
    );

    return mItems->mBuffer;
}

GameModeConfigMenu::UpdateAction SardineConfigMenu::updateMenu(int selectIndex) {
    SardineInfo* sardine = GameModeManager::instance()->getInfo<SardineInfo>();
    if (!sardine) {
        Logger::log("Unable to Load Mode info!\n");
        return UpdateAction::NOOP;
    }

    bool isMode = GameModeManager::instance()->isMode(GameMode::SARDINE);
    if (!isMode) {
        return UpdateAction::NOOP;
    }

    switch (selectIndex) {
        case 0: {
            Logger::log("Setting Gravity Mode.\n");
            sardine->mIsUseGravity = !sardine->mIsUseGravity;
            return UpdateAction::REFRESH;
        }
        case 1: {
            Logger::log("Setting Sardine Tether.\n");
            sardine->mIsTether = !sardine->mIsTether;
            return UpdateAction::REFRESH;
        }
        case 2: {
            Logger::log("Setting Sardine Tether Snap.\n");
            sardine->mIsTetherSnap = sardine->mIsTether && !sardine->mIsTetherSnap;
            return UpdateAction::REFRESH;
        }
        default: {
            Logger::log("Failed to interpret Index!\n");
            return UpdateAction::NOOP;
        }
    }
}
