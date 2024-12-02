#include "server/freeze-tag/FreezeTagConfigMenu.hpp"

#include <stdint.h>
#include "nn/util.h"

FreezeTagConfigMenu::FreezeTagConfigMenu() : GameModeConfigMenu() {
    mItems = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    mItems->mBuffer[0].copy(u"Host Mode (OFF)      ");
    mItems->mBuffer[1].copy(u"Debug Mode (OFF)     ");
    mItems->mBuffer[2].copy(u"Set Score            ");
    mItems->mBuffer[3].copy(u"Set Round Duration   ");
    mItems->mBuffer[4].copy(u"Mario Collision (ON) ");
    mItems->mBuffer[5].copy(u"Mario Bounce (OFF)   ");
    mItems->mBuffer[6].copy(u"Cappy Collision (ON) ");
    mItems->mBuffer[7].copy(u"Cappy Bounce (OFF)   ");

    mScoreKeyboard = new Keyboard(6);
    mScoreKeyboard->setHeaderText(u"Set your Freeze Tag score");
    mScoreKeyboard->setSubText(u"");

    mRoundKeyboard = new Keyboard(3);
    mRoundKeyboard->setHeaderText(u"Set length of rounds you start in minutes");
    mRoundKeyboard->setSubText(u"This will be automatically sent to other players (2-60 minutes)");
}

const sead::WFixedSafeString<0x200>* FreezeTagConfigMenu::getStringData() {
    const char16_t* host = (
        FreezeTagInfo::mIsHostMode
        ? u"Host Mode (ON)       "
        : u"Host Mode (OFF)      "
    );

    const char16_t* debug = (
        FreezeTagInfo::mIsDebugMode
        ? u"Debug Mode (ON)      "
        : u"Debug Mode (OFF)     "
    );

    const char16_t* roundDuration = u"Set Round Duration   ";

    // Collision Toggles
    const char16_t* marioCollision = (
        FreezeTagInfo::mHasMarioCollision
        ? u"Mario Collision (ON) "
        : u"Mario Collision (OFF)"
    );
    const char16_t* marioBounce = (
        FreezeTagInfo::mHasMarioBounce
        ? u"Mario Bounce (ON)    "
        : u"Mario Bounce (OFF)   "
    );
    const char16_t* cappyCollision = (
        FreezeTagInfo::mHasCappyCollision
        ? u"Cappy Collision (ON) "
        : u"Cappy Collision (OFF)"
    );
    const char16_t* cappyBounce = (
        FreezeTagInfo::mHasCappyBounce
        ? u"Cappy Bounce (ON)    "
        : u"Cappy Bounce (OFF)   "
    );

    mItems->mBuffer[0].copy(host);
    mItems->mBuffer[1].copy(debug);
    mItems->mBuffer[3].copy(FreezeTagInfo::mIsHostMode ? roundDuration  : marioCollision);
    mItems->mBuffer[4].copy(FreezeTagInfo::mIsHostMode ? marioCollision : marioBounce);
    mItems->mBuffer[5].copy(FreezeTagInfo::mIsHostMode ? marioBounce    : cappyCollision);
    mItems->mBuffer[6].copy(FreezeTagInfo::mIsHostMode ? cappyCollision : cappyBounce);
    mItems->mBuffer[7].copy(cappyBounce);

    return mItems->mBuffer;
}

GameModeConfigMenu::UpdateAction FreezeTagConfigMenu::updateMenu(int selectIndex) {
    int adjustedIndex = selectIndex + (!FreezeTagInfo::mIsHostMode && selectIndex >= 3);
    switch (adjustedIndex) {
        // Toggle Host Mode
        case 0: {
            FreezeTagInfo::mIsHostMode = !FreezeTagInfo::mIsHostMode;
            return UpdateAction::REFRESH;
        }
        // Toggle Debug Mode
        case 1: {
            FreezeTagInfo::mIsDebugMode = !FreezeTagInfo::mIsDebugMode;
            return UpdateAction::REFRESH;
        }
        // Set Score
        case 2: {
            uint16_t oldScore = FreezeTagInfo::mPlayerTagScore.mScore;
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
                FreezeTagInfo::mPlayerTagScore.mScore = newScore;
            }

            // We don't need to send the new score to other players here, because
            //   FreezeTagMode::update() checks for score changes every iteration

            return UpdateAction::NOOP;
        }
        // Set Round Duration
        case 3: {
            uint8_t oldTime = FreezeTagInfo::mRoundLength;
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
                FreezeTagInfo::mRoundLength = al::clamp(newTime, u8(2), u8(60));
            }
            return UpdateAction::NOOP;
        }
        case 4: {
            FreezeTagInfo::mHasMarioCollision = !FreezeTagInfo::mHasMarioCollision;
            return UpdateAction::REFRESH;
        }
        case 5: {
            FreezeTagInfo::mHasMarioBounce = !FreezeTagInfo::mHasMarioBounce;
            return UpdateAction::REFRESH;
        }
        case 6: {
            FreezeTagInfo::mHasCappyCollision = !FreezeTagInfo::mHasCappyCollision;
            return UpdateAction::REFRESH;
        }
        case 7: {
            FreezeTagInfo::mHasCappyBounce = !FreezeTagInfo::mHasCappyBounce;
            return UpdateAction::REFRESH;
        }
        default: {
            return UpdateAction::NOOP;
        }
    }
}
