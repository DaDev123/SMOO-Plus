#include "layouts/FreezeTagChaserSlot.h"

#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/Nerve/NerveUtil.h"

#include <cstring>

#include "server/Client.hpp"
#include "server/freeze/FreezeTagInfo.h"
#include "server/gamemode/GameModeManager.hpp"

FreezeTagChaserSlot::FreezeTagChaserSlot(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {
    al::initLayoutActor(this, initInfo, "FreezeTagChaserSlot", 0);
    mInfo = GameModeManager::instance()->getInfo<FreezeTagInfo>();

    initNerve(&NrvFreezeTagChaserSlot.End, 0);
    kill();
}

void FreezeTagChaserSlot::init(int index) {
    // Place slot based on index and hide
    al::setPaneLocalTrans(this, "ChaserSlot", {580.f, 270.f - (index * 55.f), 0.f});
    al::hidePane(this, "ChaserSlot");

    // Set temporary name string
    al::setPaneString(this, "TxtChaserName", u"MaxLengthNameAaa", 0);

    mChaserIndex = index;
    return;
}

void FreezeTagChaserSlot::appear() {
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &NrvFreezeTagChaserSlot.Appear);
    al::LayoutActor::appear();
}

bool FreezeTagChaserSlot::tryEnd() {
    if (!al::isNerve(this, &NrvFreezeTagChaserSlot.End)) {
        al::setNerve(this, &NrvFreezeTagChaserSlot.End);
        return true;
    }
    return false;
}

bool FreezeTagChaserSlot::tryStart() {
    if (!al::isNerve(this, &NrvFreezeTagChaserSlot.Wait) && !al::isNerve(this, &NrvFreezeTagChaserSlot.Appear)) {
        appear();
        return true;
    }

    return false;
}

void FreezeTagChaserSlot::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &NrvFreezeTagChaserSlot.Wait);
    }
}

void FreezeTagChaserSlot::exeWait() {
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

    if (!mIsVisible)  // If icon isn't visible, end wait processing here
        return;

    // Update name info in this slot
    if (mIsPlayer) {
        setSlotName(Client::instance()->getClientName());
        setSlotScore(mInfo->mPlayerTagScore.mScore);
    } else {
        if (mChaserIndex >= mInfo->mChaserPlayers.size() + !mInfo->mIsPlayerRunner)
            return;

        setSlotName(mInfo->mChaserPlayers.at(mChaserIndex - !mInfo->mIsPlayerRunner)->puppetName);
        setSlotScore(mInfo->mChaserPlayers.at(mChaserIndex - !mInfo->mIsPlayerRunner)->freezeTagScore);
    }
}

void FreezeTagChaserSlot::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void FreezeTagChaserSlot::showSlot() {
    mIsVisible = true;
    al::showPane(this, "ChaserSlot");
}

void FreezeTagChaserSlot::hideSlot() {
    mIsVisible = false;
    al::hidePane(this, "ChaserSlot");
}
// namespace