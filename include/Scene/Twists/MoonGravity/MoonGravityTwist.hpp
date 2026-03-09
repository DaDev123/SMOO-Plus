#pragma once

#include "game/Player/PlayerActorHakoniwa.h"
class MoonGravityTwist {
public:
    static void toggle();
    static void update(PlayerActorHakoniwa* p1);

    static bool sMoonGravityEnabled;
};