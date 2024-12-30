#include "layouts/InfectionIcon.h"
#include <cstdio>
#include <cstring>
#include "puppets/PuppetInfo.h"
#include "al/string/StringTmp.h"
#include "prim/seadSafeString.h"
#include "server/inf/InfectionMode.hpp"
#include "server/Client.hpp"
#include "al/util.hpp"
#include "logger.hpp"
#include "rs/util.hpp"
#include "main.hpp"

InfectionIcon::InfectionIcon(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {

    al::initLayoutActor(this, initInfo, "InfectionIcon", 0);

    mInfo = GameModeManager::instance()->getInfo<InfectionInfo>();

    initNerve(&nrvInfectionIconEnd, 0);

    al::hidePane(this, "SeekingIcon");
    al::hidePane(this, "HidingIcon");

    
    kill();

}

void InfectionIcon::appear() {

    al::startAction(this, "Appear", 0);

    al::setNerve(this, &nrvInfectionIconAppear);

    al::LayoutActor::appear();
}

bool InfectionIcon::tryEnd() {
    if (!al::isNerve(this, &nrvInfectionIconEnd)) {
        al::setNerve(this, &nrvInfectionIconEnd);
        return true;
    }
    return false;
}

bool InfectionIcon::tryStart() {

    if (!al::isNerve(this, &nrvInfectionIconWait) && !al::isNerve(this, &nrvInfectionIconAppear)) {

        appear();

        return true;
    }

    return false;
}

void InfectionIcon::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvInfectionIconWait);
    }
}

void InfectionIcon::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    

    int playerCount = Client::getMaxPlayerCount();

    if (playerCount > 0) {

        char playerNameBuf[0x100] = {0}; // max of 16 player names if player name size is 0x10

        sead::BufferedSafeStringBase<char> playerList =
            sead::BufferedSafeStringBase<char>(playerNameBuf, 0x200);
        
        // Add your own name to the list at the top
        playerList.appendWithFormat("%s %s\n", mInfo->mIsPlayerIt ? "金" : "音", Client::instance()->getClientName());

        // Add all it players to list
        for(int i = 0; i < playerCount; i++){
            PuppetInfo* curPuppet = Client::getPuppetInfo(i);
            if (curPuppet && curPuppet->isConnected && curPuppet->isIt)
                playerList.appendWithFormat("%s %s\n", curPuppet->isIt ? "金" : "音", curPuppet->puppetName);
        }

        // Add not it players to list
        for(int i = 0; i < playerCount; i++){
            PuppetInfo* curPuppet = Client::getPuppetInfo(i);
            if (curPuppet && curPuppet->isConnected && !curPuppet->isIt)
                playerList.appendWithFormat("%s %s\n", curPuppet->isIt ? "金" : "音", curPuppet->puppetName);
        }
        
        al::setPaneStringFormat(this, "TxtPlayerList", playerList.cstr());
    }
    
}

void InfectionIcon::exeEnd() {

    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void InfectionIcon::showHiding() {
    al::hidePane(this, "SeekingIcon");
    al::showPane(this, "HidingIcon");
}

void InfectionIcon::showSeeking() {
    al::hidePane(this, "HidingIcon");
    al::showPane(this, "SeekingIcon");
}

namespace {
    NERVE_IMPL(InfectionIcon, Appear)
    NERVE_IMPL(InfectionIcon, Wait)
    NERVE_IMPL(InfectionIcon, End)
}