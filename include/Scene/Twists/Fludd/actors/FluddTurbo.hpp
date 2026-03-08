#pragma once

#include <sead/basis/seadTypes.h>
#include <sead/container/seadPtrArray.h>
#include <sead/heap/seadHeap.h>
#include <sead/math/seadVector.h>

#include "al/Library/Collision/PartsConnectorUtil.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/Nerve/NerveSetupUtil.h"
#include "al/Library/Thread/FunctorV0M.h"

#include "game/Player/HackCap/HackCapJointControlKeeper.h"
#include "game/Player/PlayerActorHakoniwa.h"

#include "logger.hpp"

namespace ca {

class FluddTurbo : public al::LiveActor {
public:
    FluddTurbo(const char* name);
    void init(al::ActorInitInfo const& info) override;
    void listenAppear();
    bool receiveMsg(const al::SensorMsg* message, al::HitSensor* source, al::HitSensor* target) override;
    void attackSensor(al::HitSensor* source, al::HitSensor* target) override;
    void control() override;

    void activate(bool startEffect);
    void deactivate();
    void connect(al::LiveActor* f);
    void exeWait();

    bool isConnecting = false;

private:
    al::MtxConnector* mtxConnector = nullptr;
    sead::Quatf* quat = new sead::Quatf(0, 0, 0, 0);
    sead::Vector3f* posOffset = new sead::Vector3f(1, -1, 0);
    int effectDelay = 0;
};

}  // namespace ca