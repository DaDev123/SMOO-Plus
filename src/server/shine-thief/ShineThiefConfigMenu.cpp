#include "server/shine-thief/ShineThiefConfigMenu.hpp"

#include <stdint.h>

#include "heap/seadHeapMgr.h"
#include "Layout/CommonVerticalList.h"
#include "Library/Layout/LayoutActionFunction.h"
#include "Library/Memory/HeapUtil.h"
#include "Library/Play/Layout/RollParts.h"
#include "Scene/StageSceneStateModConfig.hpp"
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

void ShineThiefConfigMenu::initMenu() {
    ShineThiefInfo* curMode = GameModeManager::instance()->getInfo<ShineThiefInfo>();
    StageSceneStateModConfig::setMenuItemBase(mList->mListPartsArr[1]);
    StageSceneStateModConfig::setMenuItemBase(mList->mListPartsArr[2]);
    StageSceneStateModConfig::setMenuItemCheck(mList->mListPartsArr[3]);
    StageSceneStateModConfig::setMenuItemRoll(mList->mListPartsArr[4]);
    sead::ScopedCurrentHeapSetter setter(al::getSceneHeap());
    RollPartsData* empty = new RollPartsData(0, new const char16_t* [] { u"" });
    RollPartsData* teams = new RollPartsData(2, new const char16_t* [] { u"Team 1", u"Team 2" }, 0, false);
    mList->startLoopActionAll("Loop", "Loop");
    mList->setRollPartsData(new RollPartsData[]{*empty, *empty, *empty, *teams});
}

const sead::WFixedSafeString<0x200>* ShineThiefConfigMenu::getStringData() {
    ShineThiefInfo* curMode = GameModeManager::instance()->getInfo<ShineThiefInfo>();

    mItems[0].copy(u"Set Score");
    mItems[1].copy(u"Config Host Controls");

    if (curMode) {
        mItems[2].copy(u"Team Mode");
        mItems[3].copy(u"My Team");
    }

    return mItems.mBuffer;
}

void ShineThiefConfigMenu::updateDataFromRollParts() {
    if (!GameModeManager::instance()->isMode(GameMode::SHINETHIEF))
        return;

    ShineThiefMode* mode = GameModeManager::instance()->getMode<ShineThiefMode>();
    if (!mode)
        return;

    ShineThiefInfo* modeInf = GameModeManager::instance()->getInfo<ShineThiefInfo>();
    if (!modeInf)
        return;

    int team = ((al::RollParts*)mList->mListPartsArr[4])->mSelectedIdx;
    modeInf->mPlayerTeam = (ShineThiefTeam)(team + 1);

    mode->sendShineThiefPacket(ShineThiefUpdateType::PLAYER);
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
        al::startAction(mList->mListPartsArr[3], curMode->mIsTeamMode ? "On" : "Off", "State");

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