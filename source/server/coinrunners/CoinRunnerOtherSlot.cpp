#include "server/coinrunners/CoinRunnerOtherSlot.h"

#include "al/util.hpp"
#include "puppets/PuppetInfo.h"
#include "server/gamemode/GameModeManager.hpp"
#include "server/coinrunners/CoinRunnerInfo.h"

CoinRunnerOtherSlot::CoinRunnerOtherSlot(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {
    al::initLayoutActor(this, initInfo, "CoinRunnerOtherSlot", 0);
    mInfo = GameModeManager::instance()->getInfo<CoinRunnerInfo>();

    initNerve(&nrvCoinRunnerOtherSlotEnd, 0);
    kill();
}

void CoinRunnerOtherSlot::init(int index) {
    // Place slot based on index and hide
    al::setPaneLocalTrans(this, "OtherSlot", { 0.f, 270.f - (index * 55.f), 0.f });
    al::hidePane(this, "OtherSlot");

    // Set temporary name string
    al::setPaneString(this, "TxtOtherName", u"MaxLengthNameAaa", 0);

    mOtherIndex = index;
    return;
}

void CoinRunnerOtherSlot::appear() {
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &nrvCoinRunnerOtherSlotAppear);
    al::LayoutActor::appear();
}

bool CoinRunnerOtherSlot::tryEnd() {
    if (!al::isNerve(this, &nrvCoinRunnerOtherSlotEnd)) {
        al::setNerve(this, &nrvCoinRunnerOtherSlotEnd);
        return true;
    }
    return false;
}

bool CoinRunnerOtherSlot::tryStart() {
    if (!al::isNerve(this, &nrvCoinRunnerOtherSlotWait) && !al::isNerve(this, &nrvCoinRunnerOtherSlotAppear)) {
        appear();
        return true;
    }

    return false;
}

void CoinRunnerOtherSlot::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvCoinRunnerOtherSlotWait);
    }
}

void CoinRunnerOtherSlot::exeWait() {
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

void CoinRunnerOtherSlot::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void CoinRunnerOtherSlot::showSlot() {
    mIsVisible = true;
    al::showPane(this, "OtherSlot");
}

void CoinRunnerOtherSlot::hideSlot() {
    mIsVisible = false;
    al::hidePane(this, "OtherSlot");
}

namespace {
    NERVE_IMPL(CoinRunnerOtherSlot, Appear)
    NERVE_IMPL(CoinRunnerOtherSlot, Wait)
    NERVE_IMPL(CoinRunnerOtherSlot, End)
}
