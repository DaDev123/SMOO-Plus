#include "layouts/ShineThiefChaserSlot.h"

#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/Nerve/NerveUtil.h"

#include <cstring>

#include "server/Client.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/shine-thief/ShineThiefInfo.h"

ShineThiefChaserSlot::ShineThiefChaserSlot(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {
    al::initLayoutActor(this, initInfo, "ShineThiefChaserSlot", 0);
    mInfo = GameModeManager::instance()->getInfo<ShineThiefInfo>();

    initNerve(&NrvShineThiefChaserSlot.End, 0);
    kill();
}

void ShineThiefChaserSlot::init(int index) {
    al::setPaneLocalTrans(this, "ChaserSlot", {580.f, 270.f - (index * 55.f), 0.f});
    al::hidePane(this, "ChaserSlot");
    al::setPaneString(this, "TxtChaserName", u"MaxLengthNameAaa", 0);

    mChaserIndex = index;
    return;
}

void ShineThiefChaserSlot::appear() {
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &NrvShineThiefChaserSlot.Appear);
    al::LayoutActor::appear();
}

bool ShineThiefChaserSlot::tryEnd() {
    if (!al::isNerve(this, &NrvShineThiefChaserSlot.End)) {
        al::setNerve(this, &NrvShineThiefChaserSlot.End);
        return true;
    }
    return false;
}

bool ShineThiefChaserSlot::tryStart() {
    if (!al::isNerve(this, &NrvShineThiefChaserSlot.Wait) && !al::isNerve(this, &NrvShineThiefChaserSlot.Appear)) {
        appear();
        return true;
    }

    return false;
}

void ShineThiefChaserSlot::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &NrvShineThiefChaserSlot.Wait);
    }
}

void ShineThiefChaserSlot::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    if (mInfo->mIsTeamMode) {
        updateTeamSlot();
    } else {
        updateNormalSlot();
    }
}

void ShineThiefChaserSlot::updateNormalSlot() {
    mIsPlayer = mChaserIndex == 0 && !mInfo->mIsPlayerHolder;

    if (mChaserIndex >= mInfo->mThiefPlayers.size() + (!mInfo->mIsPlayerHolder ? 1 : 0)) {
        if (mIsVisible)
            hideSlot();
    } else if (!mIsVisible)
        showSlot();

    if (!mIsVisible)
        return;

    // Reset to original position for normal mode
    al::setPaneLocalTrans(this, "ChaserSlot", {580.f, 270.f - (mChaserIndex * 55.f), 0.f});

    // Show the chaser icon in normal mode
    al::showPane(this, "PicChaserIcon");

    if (mIsPlayer) {
        setSlotName(Client::instance()->getClientName());
        setSlotScore(mInfo->mPlayerTagScore.mScore);
    } else {
        if (mChaserIndex >= mInfo->mThiefPlayers.size() + (!mInfo->mIsPlayerHolder ? 1 : 0))
            return;

        int puppetIndex = mChaserIndex - (!mInfo->mIsPlayerHolder ? 1 : 0);
        setSlotName(mInfo->mThiefPlayers.at(puppetIndex)->puppetName);
        setSlotScore(mInfo->mThiefPlayers.at(puppetIndex)->shineThiefScore);
    }
}

void ShineThiefChaserSlot::updateTeamSlot() {
    uint8_t playerTeam = (uint8_t)mInfo->mPlayerTeam;

    int team1Count = mInfo->mTeam1Players.size() + (playerTeam == 1 ? 1 : 0);
    int team2Count = mInfo->mTeam2Players.size() + (playerTeam == 2 ? 1 : 0);

    bool isTeam1Slot = mChaserIndex < team1Count;
    bool isTeam2Slot = !isTeam1Slot && (mChaserIndex - team1Count) < team2Count;

    if (!isTeam1Slot && !isTeam2Slot) {
        if (mIsVisible)
            hideSlot();
        return;
    }

    float xPos;
    float yPos;

    if (isTeam1Slot) {
        // Team 1: Position to the left under Team1 header
        xPos = -500.f;
        yPos = 225.f - (mChaserIndex * 55.f);
    } else {
        // Team 2: Keep original position under Team2 header
        int team2Index = mChaserIndex - team1Count;
        xPos = 580.f;
        yPos = 225.f - (team2Index * 55.f);
    }

    al::setPaneLocalTrans(this, "ChaserSlot", {xPos, yPos, 0.f});

    // Hide the chaser icon in team mode
    al::hidePane(this, "PicChaserIcon");

    if (!mIsVisible)
        showSlot();

    if (isTeam1Slot) {
        mIsPlayer = (mChaserIndex == 0 && playerTeam == 1);

        if (mIsPlayer) {
            setSlotName(Client::instance()->getClientName());
            setSlotScore(mInfo->mPlayerTagScore.mScore);
        } else {
            int puppetIndex = mChaserIndex - (playerTeam == 1 ? 1 : 0);
            if (puppetIndex < mInfo->mTeam1Players.size()) {
                setSlotName(mInfo->mTeam1Players.at(puppetIndex)->puppetName);
                setSlotScore(mInfo->mTeam1Players.at(puppetIndex)->shineThiefScore);
            }
        }
    } else {
        int team2Index = mChaserIndex - team1Count;
        mIsPlayer = (team2Index == 0 && playerTeam == 2);

        if (mIsPlayer) {
            setSlotName(Client::instance()->getClientName());
            setSlotScore(mInfo->mPlayerTagScore.mScore);
        } else {
            int puppetIndex = team2Index - (playerTeam == 2 ? 1 : 0);
            if (puppetIndex < mInfo->mTeam2Players.size()) {
                setSlotName(mInfo->mTeam2Players.at(puppetIndex)->puppetName);
                setSlotScore(mInfo->mTeam2Players.at(puppetIndex)->shineThiefScore);
            }
        }
    }
}

void ShineThiefChaserSlot::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void ShineThiefChaserSlot::showSlot() {
    mIsVisible = true;
    al::showPane(this, "ChaserSlot");
}

void ShineThiefChaserSlot::hideSlot() {
    mIsVisible = false;
    al::hidePane(this, "ChaserSlot");
}