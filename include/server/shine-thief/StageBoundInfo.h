#pragma once

#include "Library/LiveActor/LiveActor.h"

class StageBoundInfo {
public:
    void init(al::LiveActor* p1, float checkRange, float checkInterval);

    sead::Vector3f mMin;
    sead::Vector3f mMax;
    float mPeakHeight;
    bool isInit = false;
};