#pragma once

#include "al/LiveActor/LiveActor.h"
#include "al/scene/Scene.h"
#include "al/util.hpp"
#include "al/sensor/SensorMsg.h"
#include "al/util/NerveUtil.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "rs/util/SensorUtil.h"
#include "logger.hpp"


class HotPotatoPlayerBlock : public al::LiveActor
{
public:
    HotPotatoPlayerBlock(const char *name);
    void init(al::ActorInitInfo const &) override;
    void initAfterPlacement(void) override;
    bool receiveMsg(const al::SensorMsg *message, al::HitSensor *source, al::HitSensor *target) override;
    void attackSensor(al::HitSensor *source, al::HitSensor *target) override;
    void control(void) override;
    void appear() override;

    void end();

    void exeAppear();
    void exeWait();
    void exeDisappear();
    void exeDead();

    bool mIsLocked = false;
    float mDitheringOffset = -150.f; // -150 is fully opaque, 0 is fully dithered, -80 is good looking
};

namespace
{
    NERVE_HEADER(HotPotatoPlayerBlock, Appear)
    NERVE_HEADER(HotPotatoPlayerBlock, Wait)
    NERVE_HEADER(HotPotatoPlayerBlock, Disappear)
    NERVE_HEADER(HotPotatoPlayerBlock, Dead)
}
