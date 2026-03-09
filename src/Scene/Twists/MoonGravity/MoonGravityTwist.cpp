#include "Scene/Twists/MoonGravity/MoonGravityTwist.hpp"

#include "game/Player/PlayerInfo.h"

#include "Scene/Twists/CustomPlayerConst.h"

bool MoonGravityTwist::sMoonGravityEnabled = false;

void MoonGravityTwist::toggle() {
    sMoonGravityEnabled = !sMoonGravityEnabled;
}

void MoonGravityTwist::update(PlayerActorHakoniwa* p1) {
    if (!p1 || !p1->mConst || !p1->mInfo)
        return;

    bool isMoon = p1->mInfo->mIsMoon;

    if (sMoonGravityEnabled) {
        CustomPlayerConst::setMoonGravityConst(p1->mConst);
        if (isMoon)
            CustomPlayerConst::setNormalMarioConst(p1->mConst);
    } else {
        CustomPlayerConst::setNormalMarioConst(p1->mConst);
        if (isMoon)
            CustomPlayerConst::setMoonGravityConst(p1->mConst);
    }
}