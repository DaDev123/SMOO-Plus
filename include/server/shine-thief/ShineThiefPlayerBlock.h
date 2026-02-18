#pragma once

#include "al/Library/HitSensor/SensorMsgSetupUtil.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/Nerve/NerveSetupUtil.h"

#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Util/PlayerUtil.h"

class ShineThiefPlayerBlock : public al::LiveActor {
public:
    ShineThiefPlayerBlock(const char* name);
    void init(al::ActorInitInfo const&) override;
    void initAfterPlacement(void) override;
    bool receiveMsg(const al::SensorMsg* message, al::HitSensor* source, al::HitSensor* target) override;
    void attackSensor(al::HitSensor* source, al::HitSensor* target) override;
    void control(void) override;
    void appear() override;

    void end();

    void updateRotation();
    void updateMaterials();

    void exeAppear();
    void exeWait();
    void exeDisappear();
    void exeDead();

    bool mIsLocked = false;
    float mDitheringOffset = -150.f;  // -150 is fully opaque, 0 is fully dithered, -80 is good looking
    float mRotationAngle = 0.0f;
};

namespace {
NERVE_IMPL(ShineThiefPlayerBlock, Appear)
NERVE_IMPL(ShineThiefPlayerBlock, Wait)
NERVE_IMPL(ShineThiefPlayerBlock, Disappear)
NERVE_IMPL(ShineThiefPlayerBlock, Dead)

NERVES_MAKE_STRUCT(ShineThiefPlayerBlock, Appear, Wait, Disappear, Dead)
}  // namespace