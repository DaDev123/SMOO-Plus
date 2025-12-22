#include "layouts/HideAndSeekIcon.h"
#include <cstring>
#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/Layout/LayoutActorUtil.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "server/Client.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/hns/HideAndSeekMode.hpp"

HideAndSeekIcon::HideAndSeekIcon(const char* name, const al::LayoutInitInfo& initInfo)
    : al::LayoutActor(name) {
    al::initLayoutActor(this, initInfo, "HideAndSeekIcon", 0);

    mInfo = GameModeManager::instance()->getInfo<HideAndSeekInfo>();

    // Initialize player slots
    mPlayerSlots.tryAllocBuffer(mMaxPlayers, al::getSceneHeap());
    for (int i = 0; i < mMaxPlayers; i++) {
        GameModePlayerSlot* newSlot = new (al::getSceneHeap())
            GameModePlayerSlot("PlayerSlot", initInfo, GameModePlayerSlotMode::HideAndSeek);
        newSlot->init(i);
        mPlayerSlots.pushBack(newSlot);
    }

    initNerve(&NrvHideAndSeekIcon.End, 0);

    al::hidePane(this, "SeekingIcon");
    al::hidePane(this, "HidingIcon");

    kill();
}

void HideAndSeekIcon::appear() {
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &NrvHideAndSeekIcon.Appear);

    // Start all player slots
    for (int i = 0; i < mMaxPlayers; i++)
        mPlayerSlots.at(i)->tryStart();

    al::LayoutActor::appear();
}

bool HideAndSeekIcon::tryEnd() {
    if (!al::isNerve(this, &NrvHideAndSeekIcon.End)) {
        al::setNerve(this, &NrvHideAndSeekIcon.End);

        // End all player slots
        for (int i = 0; i < mMaxPlayers; i++)
            mPlayerSlots.at(i)->tryEnd();

        return true;
    }
    return false;
}

bool HideAndSeekIcon::tryStart() {
    if (!al::isNerve(this, &NrvHideAndSeekIcon.Wait) &&
        !al::isNerve(this, &NrvHideAndSeekIcon.Appear)) {
        appear();
        return true;
    }
    return false;
}

void HideAndSeekIcon::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &NrvHideAndSeekIcon.Wait);
    }
}

void HideAndSeekIcon::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    // Update timer display
    GameTime& curTime = mInfo->mHidingTime;

    if (curTime.mHours > 0) {
        al::setPaneStringFormat(this, "TxtCounter", "%01d:%02d:%02d", curTime.mHours,
                                curTime.mMinutes, curTime.mSeconds);
    } else {
        al::setPaneStringFormat(this, "TxtCounter", "%02d:%02d", curTime.mMinutes,
                                curTime.mSeconds);
    }
}

void HideAndSeekIcon::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void HideAndSeekIcon::showHiding() {
    al::hidePane(this, "SeekingIcon");
    al::showPane(this, "HidingIcon");
}

void HideAndSeekIcon::showSeeking() {
    al::hidePane(this, "HidingIcon");
    al::showPane(this, "SeekingIcon");
}
