#include "server/freeze/FreezeTagConfigMenu.hpp"
#include <cmath>
#include <stdint.h>
#include "Keyboard.hpp"
#include "logger.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/freeze/FreezeTagMode.hpp"
#include "server/Client.hpp"

FreezeTagConfigMenu::FreezeTagConfigMenu() : GameModeConfigMenu() {
    mScoreKeyboard = new Keyboard(6);
    mScoreKeyboard->setHeaderText(u"Set your Freeze Tag score");
    mScoreKeyboard->setSubText(u"Must be in game and have the game mode active to set your score");

    mRoundKeyboard = new Keyboard(3);
    mRoundKeyboard->setHeaderText(u"Set length of rounds in minutes");
    mRoundKeyboard->setSubText(u"This length will be automatically sent to other players (max of 60 minutes)");
}

void FreezeTagConfigMenu::initMenu(const al::LayoutInitInfo &initInfo) {}

const sead::WFixedSafeString<0x200> *FreezeTagConfigMenu::getStringData() {
    sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>* gamemodeConfigOptions =
        new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();

    gamemodeConfigOptions->mBuffer[0].copy(u"Set Score");
    gamemodeConfigOptions->mBuffer[1].copy(u"Config Host Controls");
    gamemodeConfigOptions->mBuffer[2].copy(u"Toggle Host Mode");

    return gamemodeConfigOptions->mBuffer;
}

bool FreezeTagConfigMenu::updateMenu(int selectIndex) {

    FreezeTagInfo *curMode = GameModeManager::instance()->getInfo<FreezeTagInfo>();

    Logger::log("Updating freeze tag menu\n");

    if (!curMode) {
        Logger::log("Unable to Load Mode info!\n");
        return true;   
    }
    
    switch (selectIndex) {
        case 0: {
            if (GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG)) {
                uint16_t oldScore = GameModeManager::instance()->getInfo<FreezeTagInfo>()->mPlayerTagScore.mScore;
                uint16_t newScore = -1;

                // opens swkbd with the initial text set to the last saved port
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
                        if(!mScoreKeyboard->isKeyboardCancelled())
                            newScore = ::atoi(mScoreKeyboard->getResult());
                        break;
                    }
                    nn::os::YieldThread(); // allow other threads to run
                }
                
                if(newScore != uint16_t(-1))
                    curMode->mPlayerTagScore.mScore = newScore;
            }
            return true;
        }
        case 1: {
            if (GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG)) {
                curMode->mIsHostMode = true;

                uint8_t oldTime = curMode->mRoundLength;
                uint8_t newTime = -1;

                // opens swkbd with the initial text set to the last saved port
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
                        if(!mRoundKeyboard->isKeyboardCancelled())
                            newTime = ::atoi(mRoundKeyboard->getResult());
                        break;
                    }
                    nn::os::YieldThread(); // allow other threads to run
                }
                
                if(newTime != uint8_t(-1))
                    curMode->mRoundLength = al::clamp(newTime, u8(2), u8(60));
            }
            return true;
        }
        case 2: {
            if (GameModeManager::instance()->isMode(GameMode::FREEZETAG)) {
                curMode->mIsHostMode = true;
            }
            return true;
        }
        default:
            Logger::log("Failed to interpret Index!\n");
            return false;
    }
    
}