#pragma once

#include "sead/time/seadTickTime.h"

struct Time {
    static void calcTime();

    static sead::TickTime prevTime;
    static sead::TickSpan deltaSpan;
    static float deltaTime;
};
