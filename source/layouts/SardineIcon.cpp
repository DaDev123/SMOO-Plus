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

SardineIcon::SardineIcon(const char* name, const al::LayoutInitInfo& initInfo)
    : al::LayoutActor(name)
{
    al::initLayoutActor(this, initInfo, "SardineIcon", 0);

    mInfo = GameModeManager::instance()->getInfo<SardineInfo>();

    // Initialize player slots
    mPlayerSlots.tryAllocBuffer(mMaxPlayers, al::getSceneHeap());
    for (int i = 0; i < mMaxPlayers; i++) {
        GameModePlayerSlot* newSlot = new (al::getSceneHeap()) 
            GameModePlayerSlot("PlayerSlot", initInfo, GameModePlayerSlotMode::Sardine);
        newSlot->init(i);
        mPlayerSlots.pushBack(newSlot);
    }

    initNerve(&nrvSardineIconEnd, 0);

    al::hidePane(this, "SoloIcon");
    al::hidePane(this, "PackIcon");

    kill();
}

void SardineIcon::appear()
{
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &nrvSardineIconAppear);

    // Start all player slots
    for (int i = 0; i < mMaxPlayers; i++)
        mPlayerSlots.at(i)->tryStart();

    al::LayoutActor::appear();
}

bool SardineIcon::tryEnd()
{
    if (!al::isNerve(this, &nrvSardineIconEnd)) {
        al::setNerve(this, &nrvSardineIconEnd);
        
        // End all player slots
        for (int i = 0; i < mMaxPlayers; i++)
            mPlayerSlots.at(i)->tryEnd();
        
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

void SardineIcon::exeWait()
{
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    // Update timer display
    GameTime& curTime = mInfo->mHidingTime;

    if (curTime.mHours > 0) {
        al::setPaneStringFormat(this, "TxtCounter", "%01d:%02d:%02d", 
            curTime.mHours, curTime.mMinutes, curTime.mSeconds);
    } else {
        al::setPaneStringFormat(this, "TxtCounter", "%02d:%02d", 
            curTime.mMinutes, curTime.mSeconds);
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