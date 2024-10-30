#include "server/speedrun/SpeedrunIcon.h"

#include "server/gamemode/GameModeManager.hpp"
#include "al/util.hpp"

SpeedrunIcon::SpeedrunIcon(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {
    al::initLayoutActor(this, initInfo, "SpeedrunIcon", 0);

    mInfo = GameModeManager::instance()->getInfo<SpeedrunInfo>();

    initNerve(&nrvSpeedrunIconEnd, 0);

    al::hidePane(this, "SeekingIcon");
    al::hidePane(this, "HidingIcon");

    kill();
}

void SpeedrunIcon::appear() {
    al::startAction(this, "Appear", 0);

    al::setNerve(this, &nrvSpeedrunIconAppear);

    al::LayoutActor::appear();
}

bool SpeedrunIcon::tryEnd() {
    if (!al::isNerve(this, &nrvSpeedrunIconEnd)) {
        al::setNerve(this, &nrvSpeedrunIconEnd);
        return true;
    }
    return false;
}

bool SpeedrunIcon::tryStart() {
    if (!al::isNerve(this, &nrvSpeedrunIconWait) && !al::isNerve(this, &nrvSpeedrunIconAppear)) {
        appear();
        return true;
    }
    return false;
}

void SpeedrunIcon::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvSpeedrunIconWait);
    }
}

const char* SpeedrunIcon::getRoleIcon(bool isIt) {
    return isIt ? "\uE002" : "\uE001";
}

GameMode SpeedrunIcon::getGameMode() {
    return GameMode::SPEEDRUN;
}

bool SpeedrunIcon::isMeIt() {
    return mInfo->mIsPlayerIt;
}

void SpeedrunIcon::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    al::setPaneStringFormat(this, "TxtCounter", mInfo->mHidingTime.to_string().c_str());

    auto playerList = getPlayerList();
    if (!playerList.isEmpty()) {
        al::setPaneStringFormat(this, "TxtPlayerList", playerList.cstr());
    }
}

void SpeedrunIcon::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void SpeedrunIcon::showHiding() {
    al::hidePane(this, "SeekingIcon");
    al::showPane(this, "HidingIcon");
}

void SpeedrunIcon::showSeeking() {
    al::hidePane(this, "HidingIcon");
    al::showPane(this, "SeekingIcon");
}

namespace {
    NERVE_IMPL(SpeedrunIcon, Appear)
    NERVE_IMPL(SpeedrunIcon, Wait)
    NERVE_IMPL(SpeedrunIcon, End)
}
