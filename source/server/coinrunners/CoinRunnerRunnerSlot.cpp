#include "server/coinrunners/CoinRunnerRunnerSlot.h"

#include "al/util.hpp"
#include "puppets/PuppetInfo.h"
#include "server/Client.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/coinrunners/CoinRunnerInfo.h"

CoinRunnerRunnerSlot::CoinRunnerRunnerSlot(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {
    al::initLayoutActor(this, initInfo, "CoinRunnerRunnerSlot", 0);
    mInfo = GameModeManager::instance()->getInfo<CoinRunnerInfo>();

    initNerve(&nrvCoinRunnerRunnerSlotEnd, 0);
    kill();
}

void CoinRunnerRunnerSlot::init(int index) {
    // Place slot based on index and hide
    al::setPaneLocalTrans(this, "RunnerSlot", { -580.f, 270.f - (index * 55.f), 0.f });
    al::hidePane(this, "RunnerSlot");

    // Set temporary name string
    al::setPaneString(this, "TxtRunnerName", u"MaxLengthNameAaa", 0);

    mRunnerIndex = index;
    return;
}

void CoinRunnerRunnerSlot::appear() {
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &nrvCoinRunnerRunnerSlotAppear);
    al::LayoutActor::appear();
}

bool CoinRunnerRunnerSlot::tryEnd() {
    if (!al::isNerve(this, &nrvCoinRunnerRunnerSlotEnd)) {
        al::setNerve(this, &nrvCoinRunnerRunnerSlotEnd);
        return true;
    }
    return false;
}

bool CoinRunnerRunnerSlot::tryStart() {
    if (!al::isNerve(this, &nrvCoinRunnerRunnerSlotWait) && !al::isNerve(this, &nrvCoinRunnerRunnerSlotAppear)) {
        appear();
        return true;
    }

    return false;
}

void CoinRunnerRunnerSlot::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvCoinRunnerRunnerSlotWait);
    }
}

void CoinRunnerRunnerSlot::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    mIsPlayer = mRunnerIndex == 0 && mInfo->isPlayerRunner();

    // Show/hide icon if player doesn't exist in this slot
    if (mInfo->runners() <= mRunnerIndex) {
        if (mIsVisible) {
            hideSlot();
        }
    } else if (!mIsVisible) {
        showSlot();
    }

    if (!mIsVisible) { // If icon isn't visible, end wait processing here
        return;
    }

    mCoinIconSpin += 1.2f;
    if (mCoinIconSpin > 360.f + (mRunnerIndex * 7.5f)) {
        mCoinIconSpin -= 360.f;
    }

    setCoinAngle();

    // Update name info in this slot
    if (mIsPlayer) {
        setSlotName(Client::instance()->getClientName());
        setSlotScore(mInfo->getScore());
    } else {
        PuppetInfo* other = mInfo->mRunnerPlayers.at(mRunnerIndex - mInfo->isPlayerRunner());
        setSlotName(other->puppetName);
        setSlotScore(other->ftGetScore());
    }
}

void CoinRunnerRunnerSlot::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void CoinRunnerRunnerSlot::showSlot() {
    mIsVisible = true;
    al::showPane(this, "RunnerSlot");
}

void CoinRunnerRunnerSlot::hideSlot() {
    mIsVisible = false;
    al::hidePane(this, "RunnerSlot");
}

void CoinRunnerRunnerSlot::setCoinAngle() {
    al::setPaneLocalRotate(this, "PicRunnerCoin", { 0.f, 0.f, mCoinIconSpin + (mRunnerIndex * 7.5f) });

    if (mIsPlayer) {
        float targetSize = mInfo->isPlayerCoin() ? 1.f : 0.f;
        mInfo->mCoinIconSize = al::lerpValue(mInfo->mCoinIconSize, targetSize, 0.05f);
        al::setPaneLocalScale(this, "PicRunnerCoin", { mInfo->mCoinIconSize, mInfo->mCoinIconSize });
    } else if (mInfo->runners() <= mRunnerIndex) {
        return;
    } else {
        PuppetInfo* other = mInfo->mRunnerPlayers.at(mRunnerIndex - mInfo->isPlayerRunner());

        float targetSize = other->ftIsFrozen() ? 1.f : 0.f;
        other->freezeIconSize = al::lerpValue(other->freezeIconSize, targetSize, 0.05f);
        al::setPaneLocalScale(this, "PicRunnerCoin", { other->freezeIconSize, other->freezeIconSize });
    }
}

namespace {
    NERVE_IMPL(CoinRunnerRunnerSlot, Appear)
    NERVE_IMPL(CoinRunnerRunnerSlot, Wait)
    NERVE_IMPL(CoinRunnerRunnerSlot, End)
}
