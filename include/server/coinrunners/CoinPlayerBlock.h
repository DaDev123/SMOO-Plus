#pragma once

#include "al/sensor/SensorMsg.h"
#include "al/util/NerveUtil.h"

class CoinPlayerBlock : public al::LiveActor {
    public:
        CoinPlayerBlock(const char* name);
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

        bool  mIsLocked        = false;
        float mDitheringOffset = -150.f; // -150 is fully opaque, 0 is fully dithered, -80 is good looking
};

namespace {
    NERVE_HEADER(CoinPlayerBlock, Appear)
    NERVE_HEADER(CoinPlayerBlock, Wait)
    NERVE_HEADER(CoinPlayerBlock, Disappear)
    NERVE_HEADER(CoinPlayerBlock, Dead)
}
