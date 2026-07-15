#pragma once

#include "sead/math/seadVector.h"

#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/Nerve/NerveExecutor.h"

class WorldEndBorderKeeper : public al::NerveExecutor {
public:
    WorldEndBorderKeeper(const al::LiveActor*);
    void exeInside(void);
    void exeOutside(void);
    void exePullBack(void);
    void exeWaitBorder(void);
    void reset(void);
    void update(const sead::Vector3f&, const sead::Vector3f&, bool);
    ~WorldEndBorderKeeper();

    al::LiveActor* mActor;
    sead::Vector3f unkVec1;  // = sead::Vector3f::ex;
    sead::Vector3f unkVec2;  // = sead::Vector3f::ex;
};
