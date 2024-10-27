#include "server/freeze-tag/FreezeTagOtherSlot.h"

#include "al/util.hpp"
#include "puppets/PuppetInfo.h"
#include "server/gamemode/GameModeManager.hpp"
#include "server/freeze-tag/FreezeTagInfo.h"

FreezeTagOtherSlot::FreezeTagOtherSlot(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {
    al::initLayoutActor(this, initInfo, "FreezeTagOtherSlot", 0);
    mInfo = GameModeManager::instance()->getInfo<FreezeTagInfo>();

    initNerve(&nrvFreezeTagOtherSlotEnd, 0);
    kill();
}

void FreezeTagOtherSlot::init(int index) {
    // Place slot based on index and hide
    al::setPaneLocalTrans(this, "OtherSlot", { 0.f, 270.f - (index * 55.f), 0.f });
    al::hidePane(this, "OtherSlot");

    // Set temporary name string
    al::setPaneString(this, "TxtOtherName", u"MaxLengthNameAaa", 0);

    mOtherIndex = index;
    return;
}

void FreezeTagOtherSlot::appear() {
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &nrvFreezeTagOtherSlotAppear);
    al::LayoutActor::appear();
}

bool FreezeTagOtherSlot::tryEnd() {
    if (!al::isNerve(this, &nrvFreezeTagOtherSlotEnd)) {
        al::setNerve(this, &nrvFreezeTagOtherSlotEnd);
        return true;
    }
    return false;
}

bool FreezeTagOtherSlot::tryStart() {
    if (!al::isNerve(this, &nrvFreezeTagOtherSlotWait) && !al::isNerve(this, &nrvFreezeTagOtherSlotAppear)) {
        appear();
        return true;
    }

    return false;
}

void FreezeTagOtherSlot::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvFreezeTagOtherSlotWait);
    }
}

void FreezeTagOtherSlot::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    // Show/hide icon if player doesn't exist in this slot
    if (mInfo->others() <= mOtherIndex || mInfo->isRound()) {
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
    PuppetInfo* other = mInfo->mOtherPlayers.at(mOtherIndex);
    setSlotName(other->puppetName);
    setSlotScore(other->ftGetScore());
}

void FreezeTagOtherSlot::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void FreezeTagOtherSlot::showSlot() {
    mIsVisible = true;
    al::showPane(this, "OtherSlot");
}

void FreezeTagOtherSlot::hideSlot() {
    mIsVisible = false;
    al::hidePane(this, "OtherSlot");
}

namespace {
    NERVE_IMPL(FreezeTagOtherSlot, Appear)
    NERVE_IMPL(FreezeTagOtherSlot, Wait)
    NERVE_IMPL(FreezeTagOtherSlot, End)
}
