#pragma once

#include "al/actor/ActorInitInfo.h"
#include "al/LiveActor/LiveActor.h"
#include "puppets/PuppetInfo.h"

// TODO: Make this actor only created once per puppet, and use SubActorKeeper to create PartsModel actors for each capture

class PuppetHackActor : public al::LiveActor {
    public:
        PuppetHackActor(const char* name);
        virtual void init(al::ActorInitInfo const&) override;
        virtual void initAfterPlacement() override;
        virtual void control(void) override;
        virtual void movement(void) override;
        void initOnline(PuppetInfo* info, const char* hackType);

        void startAction(const char* actName);

        void startHackAnim(bool isOn);

    private:
        PuppetInfo* mInfo;
        sead::FixedSafeString<0x20> mHackType;
};
