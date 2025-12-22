#include "layouts/GameModePlayerSlot.h"
#include <cstring>
#include <math/seadMathCalcCommon.h>
#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/Layout/LayoutActorUtil.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerFunction.h"
#include "puppets/PuppetInfo.h"
#include "server/Client.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/hns/HideAndSeekMode.hpp"
#include "server/snh/SardineMode.hpp"

GameModePlayerSlot::GameModePlayerSlot(const char* name, const al::LayoutInitInfo& initInfo,
                                       GameModePlayerSlotMode mode)
    : al::LayoutActor(name), mMode(mode) {
    // Use the same layout file for both modes
    al::initLayoutActor(this, initInfo, "GameModePlayerSlot", 0);

    if (GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK)) {
        mMode = GameModePlayerSlotMode::HideAndSeek;
        mHnSInfo = GameModeManager::instance()->getInfo<HideAndSeekInfo>();
        // Hide Sardines-specific icons
        if (al::isExistPane(this, "PicPackIcon"))
            al::hidePane(this, "PicPackIcon");
        if (al::isExistPane(this, "PicSoloIcon"))
            al::hidePane(this, "PicSoloIcon");
    } else {
        mMode = GameModePlayerSlotMode::Sardine;
        mSardineInfo = GameModeManager::instance()->getInfo<SardineInfo>();
        // Hide Hide and Seek-specific icons
        if (al::isExistPane(this, "PicSeekerIcon"))
            al::hidePane(this, "PicSeekerIcon");
        if (al::isExistPane(this, "PicHiderIcon"))
            al::hidePane(this, "PicHiderIcon");
    }

    initNerve(&NrvGameModePlayerSlot.End, 0);
    kill();
}

void GameModePlayerSlot::init(int index) {
    al::setPaneLocalTrans(this, "PlayerSlot", {0.f, 270.f - (index * 30.f), 0.f});
    al::hidePane(this, "PlayerSlot");

    // Set temporary name string
    al::setPaneString(this, "TxtPlayerName", u"MaxLengthNameAaa", 0);

    mPlayerIndex = index;
    return;
}

void GameModePlayerSlot::appear() {
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &NrvGameModePlayerSlot.Appear);
    al::LayoutActor::appear();
}

bool GameModePlayerSlot::tryEnd() {
    if (!al::isNerve(this, &NrvGameModePlayerSlot.End)) {
        al::setNerve(this, &NrvGameModePlayerSlot.End);
        return true;
    }
    return false;
}

bool GameModePlayerSlot::tryStart() {
    if (!al::isNerve(this, &NrvGameModePlayerSlot.Wait) &&
        !al::isNerve(this, &NrvGameModePlayerSlot.Appear)) {
        appear();
        return true;
    }

    return false;
}

void GameModePlayerSlot::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &NrvGameModePlayerSlot.Wait);
    }
}

void GameModePlayerSlot::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    // Hide slot if player is dead (applies to both Hide and Seek and Sardine modes)
    if (mScene) {
        PlayerActorBase* playerBase = (PlayerActorBase*)rs::getPlayerActor(mScene);
        if (playerBase && PlayerFunction::isPlayerDeadStatus(playerBase)) {
            if (mIsVisible)
                hideSlot();
            return;
        }
    }

    int maxPlayerCount = Client::getMaxPlayerCount();

    // Get the correct "isIt" state based on game mode
    bool localPlayerIsIt = (mMode == GameModePlayerSlotMode::HideAndSeek) ? mHnSInfo->mIsPlayerIt :
                                                                            mSardineInfo->mIsIt;

    struct PlayerEntry {
        const char* name;
        PuppetInfo* puppet;
        bool isLocalPlayer;
    };

    PlayerEntry players[17];
    int playerCount = 0;

    // Always add local player first
    players[playerCount].name = Client::instance()->getClientName();
    players[playerCount].puppet = nullptr;
    players[playerCount].isLocalPlayer = true;
    playerCount++;

    // Add all "It" players
    for (int i = 0; i < maxPlayerCount; i++) {
        PuppetInfo* puppet = Client::getPuppetInfo(i);
        if (puppet && puppet->isConnected && puppet->isIt) {
            players[playerCount].name = puppet->puppetName;
            players[playerCount].puppet = puppet;
            players[playerCount].isLocalPlayer = false;
            playerCount++;
        }
    }

    // Add all "Not It" players
    for (int i = 0; i < maxPlayerCount; i++) {
        PuppetInfo* puppet = Client::getPuppetInfo(i);
        if (puppet && puppet->isConnected && !puppet->isIt) {
            players[playerCount].name = puppet->puppetName;
            players[playerCount].puppet = puppet;
            players[playerCount].isLocalPlayer = false;
            playerCount++;
        }
    }

    // Show/hide slot based on whether a player exists at this index
    if (mPlayerIndex >= playerCount) {
        if (mIsVisible)
            hideSlot();
        return;  // Exit early if slot not visible
    } else if (!mIsVisible) {
        showSlot();
    }

    // Get the player for this slot and check their CURRENT role
    const char* playerName = players[mPlayerIndex].name;
    bool isIt;

    if (players[mPlayerIndex].isLocalPlayer) {
        isIt = localPlayerIsIt;
    } else {
        isIt = players[mPlayerIndex].puppet->isIt;
    }

    // Update the slot's display
    if (playerName) {
        setSlotName(playerName);
    }

    // First, always hide all icons from the wrong game mode
    if (mMode == GameModePlayerSlotMode::HideAndSeek) {
        // Hide Sardines icons
        if (al::isExistPane(this, "PicPackIcon"))
            al::hidePane(this, "PicPackIcon");
        if (al::isExistPane(this, "PicSoloIcon"))
            al::hidePane(this, "PicSoloIcon");
    } else {
        // Hide Hide and Seek icons
        if (al::isExistPane(this, "PicSeekerIcon"))
            al::hidePane(this, "PicSeekerIcon");
        if (al::isExistPane(this, "PicHiderIcon"))
            al::hidePane(this, "PicHiderIcon");
    }

    // Determine icon pane names based on game mode
    const char* itIconName =
        (mMode == GameModePlayerSlotMode::HideAndSeek) ? "PicSeekerIcon" : "PicPackIcon";
    const char* notItIconName =
        (mMode == GameModePlayerSlotMode::HideAndSeek) ? "PicHiderIcon" : "PicSoloIcon";

    // Show/hide appropriate icon based on current role
    if (isIt) {
        // Show "It" icon, hide "Not It" icon
        if (al::isExistPane(this, itIconName)) {
            if (al::isHidePane(this, itIconName))
                al::showPane(this, itIconName);
        }

        if (al::isExistPane(this, notItIconName)) {
            if (!al::isHidePane(this, notItIconName))
                al::hidePane(this, notItIconName);
        }
    } else {
        // Show "Not It" icon, hide "It" icon
        if (al::isExistPane(this, notItIconName)) {
            if (al::isHidePane(this, notItIconName))
                al::showPane(this, notItIconName);
        }

        if (al::isExistPane(this, itIconName)) {
            if (!al::isHidePane(this, itIconName))
                al::hidePane(this, itIconName);
        }
    }
}

void GameModePlayerSlot::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void GameModePlayerSlot::showSlot() {
    mIsVisible = true;
    al::showPane(this, "PlayerSlot");
}

void GameModePlayerSlot::hideSlot() {
    mIsVisible = false;
    al::hidePane(this, "PlayerSlot");
}

void GameModePlayerSlot::setSlotName(const char* name) {
    if (name) {
        char16_t wideName[0x100];
        int i = 0;
        while (name[i] && i < 0xFF) {
            wideName[i] = static_cast<char16_t>(name[i]);
            i++;
        }
        wideName[i] = 0;
        al::setPaneString(this, "TxtPlayerName", wideName, 0);
    }
}
