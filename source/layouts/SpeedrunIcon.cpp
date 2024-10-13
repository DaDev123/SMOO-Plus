#include "layouts/SpeedrunIcon.h"
#include <cstdio>
#include <cstring>
#include "puppets/PuppetInfo.h"
#include "al/string/StringTmp.h"
#include "prim/seadSafeString.h"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/spe/SpeedrunMode.hpp"
#include "server/Client.hpp"
#include "al/util.hpp"
#include "logger.hpp"
#include "rs/util.hpp"
#include "main.hpp"

SpeedrunIcon::SpeedrunIcon(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {

    al::initLayoutActor(this, initInfo, "SpeedrunIcon", 0);

    mInfo = GameModeManager::instance()->getInfo<SpeedrunInfo>();

    initNerve(&nrvSpeedrunIconEnd, 0);

    al::hidePane(this, "SeekingIcon");
    al::hidePane(this, "HidingIcon");

    
    kill();

}

void SpeedrunIcon::appear() {

    al::startAction(this, "Appear", 0);

    al::setNerve(this, &nrvSpeedrunIconAppear);

    al::LayoutActor::appear();
}

bool SpeedrunIcon::tryEnd() {
    if (!al::isNerve(this, &nrvSpeedrunIconEnd)) {
        al::setNerve(this, &nrvSpeedrunIconEnd);
        return true;
    }
    return false;
}

bool SpeedrunIcon::tryStart() {

    if (!al::isNerve(this, &nrvSpeedrunIconWait) && !al::isNerve(this, &nrvSpeedrunIconAppear)) {

        appear();

        return true;
    }

    return false;
}

void SpeedrunIcon::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvSpeedrunIconWait);
    }
}

void SpeedrunIcon::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    GameTime &curTime = mInfo->mHidingTime;

    if (curTime.mHours > 0) {
        al::setPaneStringFormat(this, "TxtCounter", "%01d:%02d:%02d", curTime.mHours, curTime.mMinutes,
                            curTime.mSeconds);
    } else {
        al::setPaneStringFormat(this, "TxtCounter", "%02d:%02d", curTime.mMinutes,
                            curTime.mSeconds);
    }

    

    int playerCount = Client::getMaxPlayerCount();

    if (playerCount > 0) {

        char playerNameBuf[0x100] = {0}; // max of 16 player names if player name size is 0x10

        sead::BufferedSafeStringBase<char> playerList =
            sead::BufferedSafeStringBase<char>(playerNameBuf, 0x200);
        
        // Add your own name to the list at the top
        playerList.appendWithFormat("%s %s\n", mInfo->mIsPlayerIt ? "&" : "%%", Client::instance()->getClientName());

        // Add all it players to list
        for(int i = 0; i < playerCount; i++){
            PuppetInfo* curPuppet = Client::getPuppetInfo(i);
            if (curPuppet && curPuppet->isConnected && curPuppet->isIt)
                playerList.appendWithFormat("%s %s\n", curPuppet->isIt ? "&" : "%%", curPuppet->puppetName);
        }

        // Add not it players to list
        for(int i = 0; i < playerCount; i++){
            PuppetInfo* curPuppet = Client::getPuppetInfo(i);
            if (curPuppet && curPuppet->isConnected && !curPuppet->isIt)
                playerList.appendWithFormat("%s %s\n", curPuppet->isIt ? "&" : "%%", curPuppet->puppetName);
        }
        
        al::setPaneStringFormat(this, "TxtPlayerList", playerList.cstr());
    }
    
}

void SpeedrunIcon::exeEnd() {

    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void SpeedrunIcon::showHiding() {
    al::hidePane(this, "SeekingIcon");
    al::showPane(this, "HidingIcon");
}

void SpeedrunIcon::showSeeking() {
    al::hidePane(this, "HidingIcon");
    al::showPane(this, "SeekingIcon");
}

namespace {
    NERVE_IMPL(SpeedrunIcon, Appear)
    NERVE_IMPL(SpeedrunIcon, Wait)
    NERVE_IMPL(SpeedrunIcon, End)
}