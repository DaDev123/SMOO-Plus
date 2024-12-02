#include "server/freeze-tag/FreezeTagChaserSlot.h"

#include "al/string/StringTmp.h"
#include "al/util.hpp"

#include "puppets/PuppetInfo.h"

#include "server/Client.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/freeze-tag/FreezeTagInfo.h"

FreezeTagChaserSlot::FreezeTagChaserSlot(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {
    al::initLayoutActor(this, initInfo, "FreezeTagChaserSlot", 0);
    mInfo = GameModeManager::instance()->getInfo<FreezeTagInfo>();

    initNerve(&nrvFreezeTagChaserSlotEnd, 0);
    kill();
}

void FreezeTagChaserSlot::init(int index) {
    // Place slot based on index and hide
    al::setPaneLocalTrans(this, "ChaserSlot", { 580.f, 270.f - (index * 55.f), 0.f });
    al::hidePane(this, "ChaserSlot");

    // Set temporary name string
    al::setPaneString(this, "TxtChaserName", u"MaxLengthNameAaa", 0);

    mChaserIndex = index;
    return;
}

void FreezeTagChaserSlot::appear() {
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &nrvFreezeTagChaserSlotAppear);
    al::LayoutActor::appear();
}

bool FreezeTagChaserSlot::tryEnd() {
    if (!al::isNerve(this, &nrvFreezeTagChaserSlotEnd)) {
        al::setNerve(this, &nrvFreezeTagChaserSlotEnd);
        return true;
    }
    return false;
}

bool FreezeTagChaserSlot::tryStart() {
    if (!al::isNerve(this, &nrvFreezeTagChaserSlotWait) && !al::isNerve(this, &nrvFreezeTagChaserSlotAppear)) {
        appear();
        return true;
    }

    return false;
}

void FreezeTagChaserSlot::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvFreezeTagChaserSlotWait);
    }
}

void FreezeTagChaserSlot::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    mIsPlayer = mChaserIndex == 0 && mInfo->isPlayerChaser();

    // Show/hide icon if player doesn't exist in this slot
    if (mInfo->chasers() <= mChaserIndex) {
        if (mIsVisible) {
            hideSlot();
        }
    } else if (!mIsVisible) {
        showSlot();
    }

    if (!mIsVisible) { // If icon isn't visible, end wait processing here
        return;
    }

    // Update name info in this slot
    if (mIsPlayer) {
        setSlotName(Client::instance()->getClientName());
        setSlotScore(mInfo->getScore());
    } else {
        PuppetInfo* other = mInfo->mChaserPlayers.at(mChaserIndex - mInfo->isPlayerChaser());
        setSlotName(other->puppetName);
        setSlotScore(other->ftGetScore());
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

namespace {
    NERVE_IMPL(FreezeTagChaserSlot, Appear)
    NERVE_IMPL(FreezeTagChaserSlot, Wait)
    NERVE_IMPL(FreezeTagChaserSlot, End)
}
