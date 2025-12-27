#pragma once

#include "al/Library/HitSensor/SensorMsgSetupUtil.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/Nerve/NerveSetupUtil.h"

#include "game/Player/PlayerActorHakoniwa.h"

class FreezePlayerBlock : public al::LiveActor {
public:
    FreezePlayerBlock(const char* name);
    void init(al::ActorInitInfo const&) override;
    void initAfterPlacement(void) override;
    bool receiveMsg(const al::SensorMsg* message, al::HitSensor* source, al::HitSensor* target) override;
    void attackSensor(al::HitSensor* source, al::HitSensor* target) override;
    void control(void) override;
    void appear() override;

    void end();

    void exeAppear();
    void exeWait();
    void exeDisappear();
    void exeDead();

    bool mIsLocked = false;
    float mDitheringOffset = -150.f;  // -150 is fully opaque, 0 is fully dithered, -80 is good looking
};

namespace {
NERVE_IMPL(FreezePlayerBlock, Appear)
NERVE_IMPL(FreezePlayerBlock, Wait)
NERVE_IMPL(FreezePlayerBlock, Disappear)
NERVE_IMPL(FreezePlayerBlock, Dead)

NERVES_MAKE_STRUCT(FreezePlayerBlock, Appear, Wait, Disappear, Dead)
}  // namespace
