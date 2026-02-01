#pragma once

#include "al/Library/HitSensor/HitSensorKeeper.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/Nerve/NerveSetupUtil.h"

#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Player/PlayerAnimFrameCtrl.h"

#include <stdint.h>

#include "server/freeze/FreezeTagInfo.h"

class FreezeHintArrow : public al::LiveActor {
public:
    FreezeHintArrow(const char* name);
    void init(al::ActorInitInfo const&) override;
    void initAfterPlacement(void) override;
    bool receiveMsg(const al::SensorMsg* message, al::HitSensor* source, al::HitSensor* target) override;
    void attackSensor(al::HitSensor* source, al::HitSensor* target) override;
    void control(void) override;
    void appear() override;
    void end();

    void setTarget(sead::Vector3f* targetPosition) { mTargetTrans = targetPosition; };

    void exeWait();

    FreezeTagInfo* mInfo;

    PlayerActorBase* mPlayer;
    sead::Vector3f* mTargetTrans;
    sead::Vector3f* mArrowTrans;
    sead::Vector3f mActorUp = sead::Vector3f::ey;

    const float mMinDistance = 3250.f;
    float mDistance = -1.f;
    float mSize = 0.f;

    bool mIsActive = true;
    bool mIsVisible = false;
    bool mWasVisible = false;
    uint8_t mVisibilityCooldown = 0;
};

namespace {
NERVE_IMPL(FreezeHintArrow, Wait)

NERVES_MAKE_STRUCT(FreezeHintArrow, Wait)
}  // namespace
