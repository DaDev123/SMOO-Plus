#include "server/shine-thief/ShineThiefConfigMenu.hpp"

#include <stdint.h>

#include "server/gamemode/GameModeManager.hpp"
#include "server/shine-thief/ShineThiefInfo.h"
#include "server/shine-thief/ShineThiefMode.hpp"

ShineThiefConfigMenu::ShineThiefConfigMenu() : GameModeConfigMenu() {
    mScoreKeyboard = new Keyboard(6);
    if (mScoreKeyboard) {
        mScoreKeyboard->setHeaderText(u"Set your Shine Thief score");
        mScoreKeyboard->setSubText(u"");
    }

    mRoundKeyboard = new Keyboard(3);
    if (mRoundKeyboard) {
        mRoundKeyboard->setHeaderText(u"Set length of rounds in minutes");
        mRoundKeyboard->setSubText(u"");
    }
}

const sead::WFixedSafeString<0x200>* ShineThiefConfigMenu::getStringData() {
    ShineThiefInfo* curMode = GameModeManager::instance()->getInfo<ShineThiefInfo>();

    mItems[0].copy(u"Set Score");
    mItems[1].copy(u"Config Host Controls");

    if (curMode) {
        if (curMode->mIsTeamMode) {
            mItems[2].copy(u"Team Mode: ON");
        } else {
            mItems[2].copy(u"Team Mode: OFF");
        }

        if (curMode->mIsTeamMode) {
            switch (curMode->mPlayerTeam) {
            case ShineThiefTeam::TEAM_1:
                mItems[3].copy(u"My Team: Team 1");
                break;
            case ShineThiefTeam::TEAM_2:
                mItems[3].copy(u"My Team: Team 2");
                break;
            default:
                // Should never happen in team mode, but safety fallback
                mItems[3].copy(u"My Team: Team 1");
                break;
            }
        } else {
            mItems[3].copy(u"My Team: N/A");
        }
    } else {
        mItems[2].copy(u"Team Mode: OFF");
        mItems[3].copy(u"My Team: N/A");
    }

    return mItems.mBuffer;
}

GameModeConfigMenu::UpdateAction ShineThiefConfigMenu::updateMenu(int selectIndex) {
    ShineThiefInfo* curMode = GameModeManager::instance()->getInfo<ShineThiefInfo>();

    if (!curMode) {
        return GameModeConfigMenu::UpdateAction::NOOP;
    }

    switch (selectIndex) {
    case 0: {
        // Set Score
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
        // Config Host Controls
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

    case 2: {
        // Toggle Team Mode
        curMode->mIsTeamMode = !curMode->mIsTeamMode;

        if (!curMode->mIsTeamMode) {
            curMode->mPlayerTeam = ShineThiefTeam::NONE;
        } else {
            // When enabling team mode, default to Team 1 if player has no team
            if (curMode->mPlayerTeam == ShineThiefTeam::NONE) {
                curMode->mPlayerTeam = ShineThiefTeam::TEAM_1;
            }
        }

        // Send packet
        if (GameModeManager::instance()->isMode(GameMode::SHINETHIEF)) {
            ShineThiefMode* mode = GameModeManager::instance()->getMode<ShineThiefMode>();
            if (mode) {
                mode->sendShineThiefPacket(ShineThiefUpdateType::PLAYER);
            }
        }

        return GameModeConfigMenu::UpdateAction::REFRESH;
    }

    case 3: {
        // Cycle Through Teams (Team 1 <-> Team 2 only, no None)
        if (curMode->mIsTeamMode) {
            switch (curMode->mPlayerTeam) {
            case ShineThiefTeam::TEAM_1:
                curMode->mPlayerTeam = ShineThiefTeam::TEAM_2;
                break;
            case ShineThiefTeam::TEAM_2:
                curMode->mPlayerTeam = ShineThiefTeam::TEAM_1;
                break;
            default:
                // Fallback: if somehow None, set to Team 1
                curMode->mPlayerTeam = ShineThiefTeam::TEAM_1;
                break;
            }

            // Send packet
            if (GameModeManager::instance()->isMode(GameMode::SHINETHIEF)) {
                ShineThiefMode* mode = GameModeManager::instance()->getMode<ShineThiefMode>();
                if (mode) {
                    mode->sendShineThiefPacket(ShineThiefUpdateType::PLAYER);
                }
            }
        }

        return GameModeConfigMenu::UpdateAction::REFRESH;
    }

    default:
        return GameModeConfigMenu::UpdateAction::NOOP;
    }
}