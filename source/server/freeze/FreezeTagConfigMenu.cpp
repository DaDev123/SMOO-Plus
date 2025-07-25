#include "server/freeze/FreezeTagConfigMenu.hpp"
#include <cmath>
#include <stdint.h>
#include "Keyboard.hpp"
#include "logger.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/freeze/FreezeTagMode.hpp"
#include "server/Client.hpp"

static constexpr int mItemCount = 4;

FreezeTagConfigMenu::FreezeTagConfigMenu() : GameModeConfigMenu() {
    mScoreKeyboard = new Keyboard(6);
    mScoreKeyboard->setHeaderText(u"Set your Freeze Tag score");
    mScoreKeyboard->setSubText(u"Must be in game and have the game mode active to set your score");

    mRoundKeyboard = new Keyboard(3);
    mRoundKeyboard->setHeaderText(u"Set length of rounds in minutes");
    mRoundKeyboard->setSubText(u"This length will be automatically sent to other players (max of 60 minutes)");

    mConfigOptions = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    updateOptionsText();
}

void FreezeTagConfigMenu::initMenu(const al::LayoutInitInfo &initInfo) {}

void FreezeTagConfigMenu::updateOptionsText() {
    FreezeTagInfo *curMode = GameModeManager::instance()->getInfo<FreezeTagInfo>();
    // bool isDebugEnabled = curMode ? curMode->mIsDebugMode : false;
    bool isHostMode = curMode ? curMode->mIsHostMode : false;

    int index = 0;
    mConfigOptions->mBuffer[index++].copy(u"Set Score");
    mConfigOptions->mBuffer[index++].copy(isHostMode ? u"Host Mode (ON)" : u"Host Mode (OFF)");

    if (isHostMode) {
        mConfigOptions->mBuffer[index++].copy(u"Config Round Timer");
    }

    // mConfigOptions->mBuffer[index++].copy(
    //     isDebugEnabled ? u"Debug Mode (ON)" : u"Debug Mode (OFF)"
    // );
}

const sead::WFixedSafeString<0x200> *FreezeTagConfigMenu::getStringData() {
    updateOptionsText();
    return mConfigOptions->mBuffer;
}

bool FreezeTagConfigMenu::updateMenu(int selectIndex) {
    FreezeTagInfo *curMode = GameModeManager::instance()->getInfo<FreezeTagInfo>();

    Logger::log("Updating freeze tag menu\n");

    if (!curMode) {
        Logger::log("Unable to Load Mode info!\n");
        return true;
    }

    int index = 0;

    // Case 0: Set Score
    if (selectIndex == index++) {
        if (GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG)) {
            uint16_t oldScore = curMode->mPlayerTagScore.mScore;
            uint16_t newScore = -1;

            char buf[5];
            nn::util::SNPrintf(buf, 5, "%u", oldScore);

            mScoreKeyboard->openKeyboard(buf, [](nn::swkbd::KeyboardConfig& config) {
                config.keyboardMode = nn::swkbd::KeyboardMode::ModeNumeric;
                config.textMaxLength = 4;
                config.textMinLength = 1;
                config.isUseUtf8 = true;
                config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
            });

            while (true) {
                if (mScoreKeyboard->isThreadDone()) {
                    if (!mScoreKeyboard->isKeyboardCancelled())
                        newScore = ::atoi(mScoreKeyboard->getResult());
                    break;
                }
                nn::os::YieldThread();
            }

            if (newScore != uint16_t(-1))
                curMode->mPlayerTagScore.mScore = newScore;
        }
        return true;
    }

    // Case 1: Toggle Host Mode
    if (selectIndex == index++) {
        curMode->mIsHostMode = !curMode->mIsHostMode;
        Logger::log("Toggled Host Mode to: %s\n", curMode->mIsHostMode ? "ON" : "OFF");
        return true;
    }

    // Case 2: Config Host Controls (only if Host Mode is enabled)
    if (curMode->mIsHostMode && selectIndex == index++) {
        uint8_t oldTime = curMode->mRoundLength;
        uint8_t newTime = -1;

        char buf[3];
        nn::util::SNPrintf(buf, 3, "%u", oldTime);

        mRoundKeyboard->openKeyboard(buf, [](nn::swkbd::KeyboardConfig& config) {
            config.keyboardMode = nn::swkbd::KeyboardMode::ModeNumeric;
            config.textMaxLength = 2;
            config.textMinLength = 1;
            config.isUseUtf8 = true;
            config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
        });

        while (true) {
            if (mRoundKeyboard->isThreadDone()) {
                if (!mRoundKeyboard->isKeyboardCancelled())
                    newTime = ::atoi(mRoundKeyboard->getResult());
                break;
            }
            nn::os::YieldThread();
        }

        if (newTime != uint8_t(-1))
            curMode->mRoundLength = al::clamp(newTime, u8(2), u8(60));

        return true;
    }

    // Debug mode toggle (disabled)
    /*
    if (selectIndex == index++) {
        if (GameModeManager::instance()->isMode(GameMode::FREEZETAG)) {
            curMode->mIsDebugMode = !curMode->mIsDebugMode;
            Logger::log("Toggled Freeze Tag Debug Mode to: %s\n", curMode->mIsDebugMode ? "ON" : "OFF");
        }
        return true;
    }
    */

    Logger::log("Failed to interpret Index!\n");
    return false;
}
