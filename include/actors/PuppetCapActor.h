#pragma once

#include "al/Library/LiveActor/LiveActor.h"

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

    // Public access for direct manipulation
    HackCapJointControlKeeper* mJointKeeper = nullptr;

private:
    PuppetInfo* mInfo = nullptr;
    float mClosingSpeed = 0;
};