#include "layouts/HideAndSeekIcon.h"
#include <cstdio>
#include <cstring>
#include "puppets/PuppetInfo.h"
#include "al/string/StringTmp.h"
#include "prim/seadSafeString.h"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/hns/HideAndSeekMode.hpp"
#include "server/Client.hpp"
#include "al/util.hpp"
#include "logger.hpp"
#include "rs/util.hpp"
#include "main.hpp"

#define curGamemodeID 0

HideAndSeekIcon::HideAndSeekIcon(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {

    al::initLayoutActor(this, initInfo, "HideAndSeekIcon", 0);

    mInfo = GameModeManager::instance()->getInfo<HideAndSeekInfo>();

    initNerve(&nrvHideAndSeekIconEnd, 0);

    al::hidePane(this, "SeekingIcon");
    al::hidePane(this, "HidingIcon");

    
    kill();

}

void HideAndSeekIcon::appear() {

    al::startAction(this, "Appear", 0);

    al::setNerve(this, &nrvHideAndSeekIconAppear);

    al::LayoutActor::appear();
}

bool HideAndSeekIcon::tryEnd() {
    if (!al::isNerve(this, &nrvHideAndSeekIconEnd)) {
        al::setNerve(this, &nrvHideAndSeekIconEnd);
        return true;
    }
    return false;
}

bool HideAndSeekIcon::tryStart() {

    if (!al::isNerve(this, &nrvHideAndSeekIconWait) && !al::isNerve(this, &nrvHideAndSeekIconAppear)) {

        appear();

        return true;
    }

    return false;
}

void HideAndSeekIcon::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvHideAndSeekIconWait);
    }
}

void HideAndSeekIcon::exeWait() {
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
    char playerNameBuf[0x200] = {0};
    sead::BufferedSafeStringBase<char> playerList =
        sead::BufferedSafeStringBase<char>(playerNameBuf, sizeof(playerNameBuf));

    playerList.appendWithFormat("%s %s\n", mInfo->mIsPlayerIt ? "&" : "%%", Client::instance()->getClientName());

    for (int i = 0; i < playerCount; i++) {
        PuppetInfo* curPuppet = Client::getPuppetInfo(i);
        if (!curPuppet || !curPuppet->isConnected)
            continue;

        if (curPuppet->gameMode == curGamemodeID) {
            playerList.appendWithFormat("%s %s\n", curPuppet->isIt ? "&" : "%%", curPuppet->puppetName);
        }
    }

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

void HideAndSeekIcon::exeEnd() {

    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void HideAndSeekIcon::showHiding() {
    al::hidePane(this, "SeekingIcon");
    al::showPane(this, "HidingIcon");
}

void HideAndSeekIcon::showSeeking() {
    al::hidePane(this, "HidingIcon");
    al::showPane(this, "SeekingIcon");
}

namespace {
    NERVE_IMPL(HideAndSeekIcon, Appear)
    NERVE_IMPL(HideAndSeekIcon, Wait)
    NERVE_IMPL(HideAndSeekIcon, End)
}