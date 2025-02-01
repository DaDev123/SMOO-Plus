#include "layouts/HotPotatoChaserSlot.h"
#include "al/string/StringTmp.h"
#include "al/util.hpp"
#include "al/util/MathUtil.h"
#include "logger.hpp"
#include "main.hpp"
#include "math/seadVector.h"
#include "prim/seadSafeString.h"
#include "puppets/PuppetInfo.h"
#include "rs/util.hpp"
#include "server/Client.hpp"
#include "server/hotpotato/HotPotatoMode.hpp"
#include <cstdio>
#include <cstring>

HotPotatoChaserSlot::HotPotatoChaserSlot(const char* name, const al::LayoutInitInfo& initInfo)
    : al::LayoutActor(name)
{
    al::initLayoutActor(this, initInfo, "HotPotatoChaserSlot", 0);
    mInfo = GameModeManager::instance()->getInfo<HotPotatoInfo>();

    initNerve(&nrvHotPotatoChaserSlotEnd, 0);
    kill();
}

void HotPotatoChaserSlot::init(int index)
{
    // Place slot based on index and hide
    al::setPaneLocalTrans(this, "ChaserSlot", { 580.f, 270.f - (index * 55.f), 0.f });
    al::hidePane(this, "ChaserSlot");

    // Set temporary name string
    al::setPaneString(this, "TxtChaserName", u"MaxLengthNameAaa", 0);

    mChaserIndex = index;
    return;
}

void HotPotatoChaserSlot::appear()
{
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &nrvHotPotatoChaserSlotAppear);
    al::LayoutActor::appear();
}

bool HotPotatoChaserSlot::tryEnd()
{
    if (!al::isNerve(this, &nrvHotPotatoChaserSlotEnd)) {
        al::setNerve(this, &nrvHotPotatoChaserSlotEnd);
        return true;
    }
    return false;
}

bool HotPotatoChaserSlot::tryStart()
{
    if (!al::isNerve(this, &nrvHotPotatoChaserSlotWait) && !al::isNerve(this, &nrvHotPotatoChaserSlotAppear)) {
        appear();
        return true;
    }

    return false;
}

void HotPotatoChaserSlot::exeAppear()
{
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvHotPotatoChaserSlotWait);
    }
}

void HotPotatoChaserSlot::exeWait()
{
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    mIsPlayer = mChaserIndex == 0 && !mInfo->mIsPlayerRunner;

    // Show/hide icon if player doesn't exist in this slot
    if (mChaserIndex >= mInfo->mChaserPlayers.size() + !mInfo->mIsPlayerRunner) {
        if (mIsVisible)
            hideSlot();
    } else if (!mIsVisible)
        showSlot();

    if (!mIsVisible) // If icon isn't visible, end wait processing here
        return;

    // Update name info in this slot
    if (mIsPlayer) {
        setSlotName(Client::instance()->getClientName());
        setSlotScore(mInfo->mPlayerTagScore.mScore);
    } else {
        if (mChaserIndex >= mInfo->mChaserPlayers.size() + !mInfo->mIsPlayerRunner)
            return;

        setSlotName(mInfo->mChaserPlayers.at(mChaserIndex - !mInfo->mIsPlayerRunner)->puppetName);
        setSlotScore(mInfo->mChaserPlayers.at(mChaserIndex - !mInfo->mIsPlayerRunner)->hotPotatoScore);
    }
}

void HotPotatoChaserSlot::exeEnd()
{

    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void HotPotatoChaserSlot::showSlot()
{
    mIsVisible = true;
    al::showPane(this, "ChaserSlot");
}

void HotPotatoChaserSlot::hideSlot()
{
    mIsVisible = false;
    al::hidePane(this, "ChaserSlot");
}

namespace {
NERVE_IMPL(HotPotatoChaserSlot, Appear)
NERVE_IMPL(HotPotatoChaserSlot, Wait)
NERVE_IMPL(HotPotatoChaserSlot, End)
}