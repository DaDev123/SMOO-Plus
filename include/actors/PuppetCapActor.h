#pragma once

#include "al/actor/ActorInitInfo.h"
#include "al/LiveActor/LiveActor.h"
#include "al/sensor/HitSensor.h"
#include "al/sensor/SensorMsg.h"

#include "game/Player/HackCap/HackCapJointControlKeeper.h"

#include "puppets/PuppetInfo.h"

class PuppetCapActor : public al::LiveActor {
    public:
        PuppetCapActor(const char* name);
        virtual void init(al::ActorInitInfo const&) override;
        virtual void initAfterPlacement() override;
        virtual void control(void) override;
        virtual void movement(void) override;

        virtual void attackSensor(al::HitSensor*, al::HitSensor*) override;
        virtual bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;

        void initOnline(PuppetInfo* info);
        void startAction(const char* actName);
        void update();

    private:
        HackCapJointControlKeeper* mJointKeeper;
        PuppetInfo* mInfo;
};
