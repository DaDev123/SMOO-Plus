#include "server/coinrunners/CoinRunnerChaserSlot.h"

#include "al/string/StringTmp.h"
#include "al/util.hpp"

#include "puppets/PuppetInfo.h"

#include "server/Client.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/coinrunners/CoinRunnerInfo.h"

CoinRunnerChaserSlot::CoinRunnerChaserSlot(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {
    al::initLayoutActor(this, initInfo, "CoinRunnerChaserSlot", 0);
    mInfo = GameModeManager::instance()->getInfo<CoinRunnerInfo>();

    initNerve(&nrvCoinRunnerChaserSlotEnd, 0);
    kill();
}

void CoinRunnerChaserSlot::init(int index) {
    // Place slot based on index and hide
    al::setPaneLocalTrans(this, "ChaserSlot", { 580.f, 270.f - (index * 55.f), 0.f });
    al::hidePane(this, "ChaserSlot");

    // Set temporary name string
    al::setPaneString(this, "TxtChaserName", u"MaxLengthNameAaa", 0);

    mChaserIndex = index;
    return;
}

void CoinRunnerChaserSlot::appear() {
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &nrvCoinRunnerChaserSlotAppear);
    al::LayoutActor::appear();
}

bool CoinRunnerChaserSlot::tryEnd() {
    if (!al::isNerve(this, &nrvCoinRunnerChaserSlotEnd)) {
        al::setNerve(this, &nrvCoinRunnerChaserSlotEnd);
        return true;
    }
    return false;
}

bool CoinRunnerChaserSlot::tryStart() {
    if (!al::isNerve(this, &nrvCoinRunnerChaserSlotWait) && !al::isNerve(this, &nrvCoinRunnerChaserSlotAppear)) {
        appear();
        return true;
    }

    return false;
}

void CoinRunnerChaserSlot::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvCoinRunnerChaserSlotWait);
    }
}

void CoinRunnerChaserSlot::exeWait() {
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

void CoinRunnerChaserSlot::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void CoinRunnerChaserSlot::showSlot() {
    mIsVisible = true;
    al::showPane(this, "ChaserSlot");
}

void CoinRunnerChaserSlot::hideSlot() {
    mIsVisible = false;
    al::hidePane(this, "ChaserSlot");
}

namespace {
    NERVE_IMPL(CoinRunnerChaserSlot, Appear)
    NERVE_IMPL(CoinRunnerChaserSlot, Wait)
    NERVE_IMPL(CoinRunnerChaserSlot, End)
}
