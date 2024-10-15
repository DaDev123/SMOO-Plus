#include "server/freeze-tag/FreezeTagConfigMenu.hpp"
#include <cmath>
#include <stdint.h>
#include "Keyboard.hpp"
#include "logger.hpp"
#include "server/gamemode/GameMode.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/freeze-tag/FreezeTagInfo.h"

FreezeTagConfigMenu::FreezeTagConfigMenu() : GameModeConfigMenu() {
    mItems = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    mItems->mBuffer[0].copy(u"Set Score");
    mItems->mBuffer[1].copy(u"Set Round Duration");
    mItems->mBuffer[2].copy(u"Debug Mode (OFF)"); // TBD

    mScoreKeyboard = new Keyboard(6);
    mScoreKeyboard->setHeaderText(u"Set your Freeze Tag score");
    mScoreKeyboard->setSubText(u"Must be in-game and have the game mode active to set your score");

    mRoundKeyboard = new Keyboard(3);
    mRoundKeyboard->setHeaderText(u"Set length of rounds you start in minutes");
    mRoundKeyboard->setSubText(u"This will be automatically sent to other players (2-60 minutes)");
}

void FreezeTagConfigMenu::initMenu(const al::LayoutInitInfo& initInfo) {}

const sead::WFixedSafeString<0x200>* FreezeTagConfigMenu::getStringData() {
    FreezeTagInfo* freeze = GameModeManager::instance()->getInfo<FreezeTagInfo>();
    bool isMode = freeze != nullptr && GameModeManager::instance()->isMode(GameMode::FREEZETAG);

    mItems->mBuffer[2].copy(
        isMode && freeze->mIsDebugMode
        ? u"Debug Mode (ON) "
        : u"Debug Mode (OFF)"
    );

    return mItems->mBuffer;
}

GameModeConfigMenu::UpdateAction FreezeTagConfigMenu::updateMenu(int selectIndex) {
    GameModeManager* gmm = GameModeManager::instance();

    FreezeTagInfo* freeze = gmm->getInfo<FreezeTagInfo>();
    if (!freeze) {
        Logger::log("Unable to Load Mode info!\n");
        return UpdateAction::NOOP;
    }

    bool isMode = gmm->isMode(GameMode::FREEZETAG);
    if (!isMode) {
        return UpdateAction::NOOP;
    }

    switch (selectIndex) {
        // Set Score
        case 0: {
            if (gmm->isActive()) {
                uint16_t oldScore = freeze->mPlayerTagScore.mScore;
                uint16_t newScore = -1;

                char buf[5];
                nn::util::SNPrintf(buf, 5, "%u", oldScore);

                mScoreKeyboard->openKeyboard(
                    buf,
                    [](nn::swkbd::KeyboardConfig& config) {
                        config.keyboardMode  = nn::swkbd::KeyboardMode::ModeNumeric;
                        config.textMaxLength = 4;
                        config.textMinLength = 1;
                        config.isUseUtf8     = true;
                        config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
                    }
                );

                while (!mScoreKeyboard->isThreadDone()) {
                    nn::os::YieldThread(); // allow other threads to run
                }

                if (!mScoreKeyboard->isKeyboardCancelled()) {
                    newScore = ::atoi(mScoreKeyboard->getResult());
                }

                if (newScore != uint16_t(-1)) {
                    freeze->mPlayerTagScore.mScore = newScore;
                }

                // We don't need to send the new score to other players here, because
                //   FreezeTagMode::update() checks for score changes every iteration
            }
            return UpdateAction::NOOP;
        }
        // Set Round Duration
        case 1: {
            if (gmm->isActive()) {
                // Side effect: activate host mode, so that we can start/cancel rounds
                freeze->mIsHostMode = true;

                uint8_t oldTime = freeze->mRoundLength;
                uint8_t newTime = -1;

                char buf[3];
                nn::util::SNPrintf(buf, 3, "%u", oldTime);

                mRoundKeyboard->openKeyboard(
                    buf,
                    [](nn::swkbd::KeyboardConfig& config) {
                        config.keyboardMode  = nn::swkbd::KeyboardMode::ModeNumeric;
                        config.textMaxLength = 2;
                        config.textMinLength = 1;
                        config.isUseUtf8     = true;
                        config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
                    }
                );

                while (!mRoundKeyboard->isThreadDone()) {
                    nn::os::YieldThread(); // allow other threads to run
                }

                if (!mRoundKeyboard->isKeyboardCancelled()) {
                    newTime = ::atoi(mRoundKeyboard->getResult());
                }

                if (newTime != uint8_t(-1)) {
                    freeze->mRoundLength = al::clamp(newTime, u8(2), u8(60));
                }
            }
            return UpdateAction::NOOP;
        }
        // Toggle Debug Mode
        case 2: {
            freeze->mIsDebugMode = !freeze->mIsDebugMode;
            return UpdateAction::REFRESH;
        }
        default: {
            Logger::log("Failed to interpret Index!\n");
            return UpdateAction::NOOP;
        }
    }
}
