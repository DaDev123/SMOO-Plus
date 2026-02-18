#include "layouts/ShineThiefRunnerSlot.h"

#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/Math/MathUtil.h"
#include "al/Library/Nerve/NerveUtil.h"

#include <cmath>
#include <cstring>

#include "puppets/PuppetInfo.h"
#include "server/Client.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/shine-thief/ShineThiefInfo.h"

ShineThiefRunnerSlot::ShineThiefRunnerSlot(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {
    al::initLayoutActor(this, initInfo, "ShineThiefRunnerSlot", 0);
    mInfo = GameModeManager::instance()->getInfo<ShineThiefInfo>();

    initNerve(&NrvShineThiefRunnerSlot.End, 0);
    kill();

    mHolderTeam = (uint8_t)ShineThiefTeam::NONE;
}

void ShineThiefRunnerSlot::init(int index) {
    al::hidePane(this, "RunnerSlot");

    al::setPaneString(this, "TxtRunnerName", u"MaxLengthNameAaa", 0);

    mSlotScale = 0.0f;
    al::setPaneLocalScale(this, "RunnerSlot", {mSlotScale, mSlotScale});

    mRunnerIndex = index;
    mHolderTeam = (uint8_t)ShineThiefTeam::NONE;
}

void ShineThiefRunnerSlot::appear() {
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &NrvShineThiefRunnerSlot.Appear);
    al::LayoutActor::appear();
}

bool ShineThiefRunnerSlot::tryEnd() {
    if (!al::isNerve(this, &NrvShineThiefRunnerSlot.End)) {
        al::setNerve(this, &NrvShineThiefRunnerSlot.End);
        return true;
    }
    return false;
}

bool ShineThiefRunnerSlot::tryStart() {
    if (!al::isNerve(this, &NrvShineThiefRunnerSlot.Wait) && !al::isNerve(this, &NrvShineThiefRunnerSlot.Appear)) {
        appear();
        return true;
    }

    return false;
}

void ShineThiefRunnerSlot::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &NrvShineThiefRunnerSlot.Wait);
    }
}

void ShineThiefRunnerSlot::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    bool anyoneHasShine = mInfo->mIsPlayerHolder || mInfo->mHolderPlayers.size() > 0;

    if (mRunnerIndex == 0) {
        if (!mIsVisible)
            showSlot();

        float targetScale = mInfo->mIsRound ? 1.0f : 0.0f;
        mSlotScale = al::lerpValue(mSlotScale, targetScale, 0.08f);
        al::setPaneLocalScale(this, "RunnerSlot", {mSlotScale, mSlotScale});

        if (!anyoneHasShine) {
            setSlotName("No Players");
            setSlotScore(0);
            mIsPlayer = false;
            mHolderTeam = (uint8_t)ShineThiefTeam::NONE;
        } else if (mInfo->mIsPlayerHolder) {
            mIsPlayer = true;
            setSlotName(Client::instance()->getClientName());
            setSlotScore(mInfo->mPlayerTagScore.mScore);
            mHolderTeam = (uint8_t)mInfo->mPlayerTeam;
        } else if (mInfo->mHolderPlayers.size() > 0) {
            mIsPlayer = false;
            setSlotName(mInfo->mHolderPlayers.at(0)->puppetName);
            setSlotScore(mInfo->mHolderPlayers.at(0)->shineThiefScore);
            mHolderTeam = mInfo->mHolderPlayers.at(0)->shineThiefTeam;
        }

        mCaughtIconSpin += 0.03f;
        setIconRotation();
    } else {
        if (mIsVisible)
            hideSlot();
    }
}

void ShineThiefRunnerSlot::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void ShineThiefRunnerSlot::showSlot() {
    mIsVisible = true;
    al::showPane(this, "RunnerSlot");
}

void ShineThiefRunnerSlot::hideSlot() {
    mIsVisible = false;
    al::hidePane(this, "RunnerSlot");
}

void ShineThiefRunnerSlot::setIconRotation() {
    float rotation = cosf(mCaughtIconSpin) * 5.0f;

    bool showGold = true;
    bool showBlue = false;
    bool showRed = false;

    if (mInfo->mIsTeamMode) {
        showGold = false;
        if (mHolderTeam == 1)  // TEAM_1
            showBlue = true;
        else if (mHolderTeam == 2)  // TEAM_2
            showRed = true;
        else
            showGold = true;  // NONE fallback
    }

    if (showGold)
        al::showPane(this, "PicRunnerIconGold");
    else
        al::hidePane(this, "PicRunnerIconGold");

    if (showBlue)
        al::showPane(this, "PicRunnerIconBlue");
    else
        al::hidePane(this, "PicRunnerIconBlue");

    if (showRed)
        al::showPane(this, "PicRunnerIconRed");
    else
        al::hidePane(this, "PicRunnerIconRed");

    al::setPaneLocalRotate(this, "PicRunnerIconGold", {0.0f, 0.0f, rotation});
    al::setPaneLocalRotate(this, "PicRunnerIconBlue", {0.0f, 0.0f, rotation});
    al::setPaneLocalRotate(this, "PicRunnerIconRed", {0.0f, 0.0f, rotation});
}