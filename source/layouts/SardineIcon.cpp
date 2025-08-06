#include "layouts/SardineIcon.h"
#include "al/string/StringTmp.h"
#include "al/util.hpp"
#include "logger.hpp"
#include "main.hpp"
#include "prim/seadSafeString.h"
#include "puppets/PuppetInfo.h"
#include "rs/util.hpp"
#include "server/Client.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/snh/SardineMode.hpp"
#include <cstdio>
#include <cstring>

#define curGamemodeID 1

SardineIcon::SardineIcon(const char* name, const al::LayoutInitInfo& initInfo)
    : al::LayoutActor(name)
{

    al::initLayoutActor(this, initInfo, "SardineIcon", 0);

    mInfo = GameModeManager::instance()->getInfo<SardineInfo>();

    initNerve(&nrvSardineIconEnd, 0);

    al::hidePane(this, "SoloIcon");
    al::hidePane(this, "PackIcon");

    kill();
}

void SardineIcon::appear()
{

    al::startAction(this, "Appear", 0);

    al::setNerve(this, &nrvSardineIconAppear);

    al::LayoutActor::appear();
}

bool SardineIcon::tryEnd()
{
    if (!al::isNerve(this, &nrvSardineIconEnd)) {
        al::setNerve(this, &nrvSardineIconEnd);
        return true;
    }
    return false;
}

bool SardineIcon::tryStart()
{

    if (!al::isNerve(this, &nrvSardineIconWait) && !al::isNerve(this, &nrvSardineIconAppear)) {

        appear();

        return true;
    }

    return false;
}

void SardineIcon::exeAppear()
{
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvSardineIconWait);
    }
}

void SardineIcon::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    GameTime& curTime = mInfo->mHidingTime;

    if (curTime.mHours > 0) {
        al::setPaneStringFormat(this, "TxtCounter", "%01d:%02d:%02d", curTime.mHours, curTime.mMinutes, curTime.mSeconds);
    } else {
        al::setPaneStringFormat(this, "TxtCounter", "%02d:%02d", curTime.mMinutes, curTime.mSeconds);
    }

    int playerCount = Client::getMaxPlayerCount();

    if (playerCount > 0) {
        char playerNameBuf[0x200] = {0};
        sead::BufferedSafeStringBase<char> playerList = sead::BufferedSafeStringBase<char>(playerNameBuf, sizeof(playerNameBuf));

        // Add current player first
        if (mInfo->mIsIt || GameModeManager::instance()->isModeAndActive(GameMode::SARDINE)) {
            playerList.appendWithFormat("%s %s\n", mInfo->mIsIt ? "@" : "©", Client::instance()->getClientName());
        }

        // IT players (sardines) then pack
        for (int i = 0; i <= 1; i++) {
            bool isIt = i == 0;
            // Add players to the list that are in the same game mode
            for (int j = 0; j < playerCount; j++) {
                PuppetInfo* curPuppet = Client::getPuppetInfo(j);
                if (!curPuppet || !curPuppet->isConnected)
                    continue;

                if (curPuppet->gameMode != curGamemodeID)
                    continue;

                if (curPuppet->isIt != isIt)
                    continue;

                playerList.appendWithFormat("%s %s\n", curPuppet->isIt ? "@" : "©", curPuppet->puppetName);
            }
        }

        // Add some spacing before non-mode players
        bool hasNonModePlayers = false;
        for (int i = 0; i < playerCount; i++) {
            PuppetInfo* curPuppet = Client::getPuppetInfo(i);
            if (!curPuppet || !curPuppet->isConnected)
                continue;

            if (curPuppet->gameMode != curGamemodeID) {
                hasNonModePlayers = true;
                break;
            }
        }

        if (hasNonModePlayers) {
            playerList.appendWithFormat("\n"); // Add blank line for spacing
        }

        // add players not in the mode
        for (int i = 0; i < playerCount; i++) {
            PuppetInfo* curPuppet = Client::getPuppetInfo(i);
            if (!curPuppet || !curPuppet->isConnected)
                continue;

            if (curPuppet->gameMode != curGamemodeID) {
                playerList.appendWithFormat("   %s\n", curPuppet->puppetName); // no icon, indented
            }
        }

        al::setPaneStringFormat(this, "TxtPlayerList", playerList.cstr());
    }
}


void SardineIcon::exeEnd()
{

    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void SardineIcon::showSolo()
{
    al::hidePane(this, "PackIcon");
    al::showPane(this, "SoloIcon");
}

void SardineIcon::showPack()
{
    al::hidePane(this, "SoloIcon");
    al::showPane(this, "PackIcon");
}

namespace {
NERVE_IMPL(SardineIcon, Appear)
NERVE_IMPL(SardineIcon, Wait)
NERVE_IMPL(SardineIcon, End)
}