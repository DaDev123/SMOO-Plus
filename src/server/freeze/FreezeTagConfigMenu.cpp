#include "server/freeze/FreezeTagConfigMenu.hpp"

#include <stdint.h>

#include "Scene/StageSceneStateServerConfig.hpp"
#include "server/freeze/FreezeTagInfo.h"
#include "server/gamemode/GameModeManager.hpp"

FreezeTagConfigMenu::FreezeTagConfigMenu() : GameModeConfigMenu() {
    mScoreKeyboard = new Keyboard(6);
    if (mScoreKeyboard) {
        mScoreKeyboard->setHeaderText(u"Set your Freeze Tag score");
        mScoreKeyboard->setSubText(u"");
    }

    mRoundKeyboard = new Keyboard(3);
    if (mRoundKeyboard) {
        mRoundKeyboard->setHeaderText(u"Set length of rounds in minutes");
        mRoundKeyboard->setSubText(u"");
    }
}

void FreezeTagConfigMenu::initMenu() {
    StageSceneStateServerConfig::setMenuItemBase(mList->mListPartsArr[1]);
    StageSceneStateServerConfig::setMenuItemBase(mList->mListPartsArr[2]);
}

const sead::WFixedSafeString<0x200>* FreezeTagConfigMenu::getStringData() {
    mItems[0].copy(u"Set Score");
    mItems[1].copy(u"Config Host Controls");

    return mItems.mBuffer;
}

GameModeConfigMenu::UpdateAction FreezeTagConfigMenu::updateMenu(int selectIndex) {
    FreezeTagInfo* curMode = GameModeManager::instance()->getInfo<FreezeTagInfo>();

    if (!curMode) {
        return GameModeConfigMenu::UpdateAction::NOOP;
    }

    switch (selectIndex) {
    case 0: {
        if (mScoreKeyboard) {
            uint16_t oldScore = curMode->mPlayerTagScore.mScore;

            char buf[6];
            nn::util::SNPrintf(buf, 6, "%u", oldScore);

            mScoreKeyboard->openKeyboard(buf, [](nn::swkbd::KeyboardConfig& config) {
                config.keyboardMode = nn::swkbd::KeyboardMode::ModeNumeric;
                config.textMaxLength = 4;
                config.textMinLength = 1;
                config.isUseUtf8 = true;
                config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
            });

            while (!mScoreKeyboard->isThreadDone()) {
                nn::os::YieldThread();
            }

            if (!mScoreKeyboard->isKeyboardCancelled()) {
                const char* result = mScoreKeyboard->getResult();
                if (result && result[0] != '\0') {
                    int newScore = atoi(result);
                    if (newScore >= 0 && newScore <= 65535) {
                        curMode->mPlayerTagScore.mScore = (uint16_t)newScore;
                    }
                }
            }
        }
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
    case 1: {
        if (mRoundKeyboard) {
            curMode->mIsHostMode = true;

            uint8_t oldTime = curMode->mRoundLength;

            char buf[4];
            nn::util::SNPrintf(buf, 4, "%u", oldTime);

            mRoundKeyboard->openKeyboard(buf, [](nn::swkbd::KeyboardConfig& config) {
                config.keyboardMode = nn::swkbd::KeyboardMode::ModeNumeric;
                config.textMaxLength = 2;
                config.textMinLength = 1;
                config.isUseUtf8 = true;
                config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
            });

            while (!mRoundKeyboard->isThreadDone()) {
                nn::os::YieldThread();
            }

            if (!mRoundKeyboard->isKeyboardCancelled()) {
                const char* result = mRoundKeyboard->getResult();
                if (result && result[0] != '\0') {
                    int newTime = atoi(result);
                    if (newTime >= 2 && newTime <= 60) {
                        curMode->mRoundLength = (uint8_t)newTime;
                    }
                }
            }
        }
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
    default:
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
}