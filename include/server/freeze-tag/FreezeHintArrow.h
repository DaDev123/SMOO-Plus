#pragma once

#include <stdint.h>
#include "al/util.hpp"
#include "game/Player/PlayerActorBase.h"
#include "sead/math/seadVector.h"

class FreezeTagInfo;

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
        sead::Vector3f*  mTargetTrans;
        sead::Vector3f*  mArrowTrans;
        sead::Vector3f   mActorUp = sead::Vector3f::ey;

        const float mMinDistanceSq = 10562500.f; // non-squared: 3250.0
        float       mDistanceSq    = -1.f;
        float       mSize          = 0.f;

        bool    mIsActive           = true;
        bool    mIsVisible          = false;
        bool    mWasVisible         = false;
        uint8_t mVisibilityCooldown = 0;
};

namespace {
    NERVE_HEADER(FreezeHintArrow, Wait)
}
