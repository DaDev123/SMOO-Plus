#include "layouts/CustomMsg.h"
#include <cstdio>
#include <cstring>
#include "puppets/PuppetInfo.h"
#include "al/string/StringTmp.h"
#include "prim/seadSafeString.h"
#include "server/Client.hpp"
#include "al/util.hpp"
#include "logger.hpp"
#include "rs/util.hpp"
#include "main.hpp"

CustomMsg::CustomMsg(const char* text, const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {

    al::initLayoutActor(this, initInfo, text, 0);

    initNerve(&nrvCustomMsgEnd, 0);

    // Check if panes exist before using them
    mHasTxtIcon = al::isExistPane(this, "TxtIcon");
    mHasTxtRank = al::isExistPane(this, "TxtRank");
    mHasTxtName = al::isExistPane(this, "TxtName");

    // Initialize with default state only if panes exist
    if (mHasTxtIcon) {
        al::setPaneStringFormat(this, "TxtIcon", "●");
    }
    if (mHasTxtRank) {
        al::setPaneStringFormat(this, "TxtRank", "CustomMsg");
    }
    if (mHasTxtName) {
        al::setPaneStringFormat(this, "TxtName", "Ready");
    }

    kill();
}

void CustomMsg::setCustomText(const char* text) {
    if (text && strlen(text) > 0) {
        strncpy(mCustomText, text, sizeof(mCustomText) - 1);
        mCustomText[sizeof(mCustomText) - 1] = '\0'; // Ensure null termination
        mHasCustomText = true;
    } else {
        mHasCustomText = false;
        mCustomText[0] = '\0';
    }
}

void CustomMsg::appear() {
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &nrvCustomMsgAppear);
    al::LayoutActor::appear();
}

bool CustomMsg::tryStart() {
    if (mIsActive)
        return false;

    appear();
    mIsActive = true;
    return true;
}

bool CustomMsg::tryEnd() {
    if (!mIsActive)
        return false;

    kill();
    mIsActive = false;
    return true;
}

void CustomMsg::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvCustomMsgWait);
    }
}

void CustomMsg::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    // Wenn Custom-Text gesetzt ist, beide Panes ersetzen
    if (mHasCustomText) {
        if (mHasTxtName)
            al::setPaneStringFormat(this, "TxtName", mCustomText);
        if (mHasTxtRank)
            al::setPaneStringFormat(this, "TxtRank", mCustomText);
    } else {
        if (mHasTxtRank) {
            al::setPaneStringFormat(this, "TxtRank", mCustomText);
        }

        int playerCount = Client::getMaxPlayerCount();

        if (playerCount > 0 && mHasTxtName) {
            char playerNameBuf[0x300] = {0};
            sead::BufferedSafeStringBase<char> playerList =
                sead::BufferedSafeStringBase<char>(playerNameBuf, sizeof(playerNameBuf));
            
            // Add your own name first
            playerList.appendWithFormat("%s (You)", Client::instance()->getClientName());

            // Add other connected players
            int connectedCount = 0;
            for(int i = 0; i < playerCount; i++){
                PuppetInfo* curPuppet = Client::getPuppetInfo(i);
                if (curPuppet && curPuppet->isConnected) {
                    if (connectedCount == 0) {
                        playerList.appendWithFormat(", %s", curPuppet->puppetName);
                    } else {
                        playerList.appendWithFormat(", %s", curPuppet->puppetName);
                    }
                    connectedCount++;
                }
            }
            
            // Use TxtName to show the player list (truncated if too long)
            al::setPaneStringFormat(this, "TxtName", playerList.cstr());
        } else if (mHasTxtName) {
            // No other players connected
            al::setPaneStringFormat(this, "TxtName", Client::instance()->getClientName());
        }
    }
}

void CustomMsg::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void CustomMsg::showHiding() {
    // Use TxtIcon to show hiding symbol - only if pane exists
    if (mHasTxtIcon) {
        al::setPaneStringFormat(this, "TxtIcon", "🟢");
    }
    if (mHasTxtRank) {
        al::setPaneStringFormat(this, "TxtRank", "HIDING");
    }
}

void CustomMsg::showSeeking() {
    // Use TxtIcon to show seeking symbol - only if pane exists
    if (mHasTxtIcon) {
        al::setPaneStringFormat(this, "TxtIcon", "🔴");
    }
    if (mHasTxtRank) {
        al::setPaneStringFormat(this, "TxtRank", "SEEKING");
    }
}

namespace {
    NERVE_IMPL(CustomMsg, Appear)
    NERVE_IMPL(CustomMsg, Wait)
    NERVE_IMPL(CustomMsg, End)
}