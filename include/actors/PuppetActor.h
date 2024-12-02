#pragma once

#include "al/actor/ActorInitInfo.h"
#include "al/audio/AudioKeeper.h"
#include "al/LiveActor/LiveActor.h"
#include "al/sensor/HitSensor.h"
#include "al/sensor/SensorMsg.h"

#include "game/Player/PlayerModelHolder.h"

#include "actors/PuppetCapActor.h"
#include "actors/PuppetHackActor.h"
#include "layouts/NameTag.h"
#include "sead/math/seadVector.h"

#include "puppets/PuppetInfo.h"
#include "puppets/HackModelHolder.hpp"
#include "helpers.hpp"
#include "algorithms/CaptureTypes.h"

#include "server/freeze-tag/FreezePlayerBlock.h"

class PuppetActor : public al::LiveActor {
    public:
        PuppetActor(const char* name);
        virtual void init(al::ActorInitInfo const&) override;
        virtual void initAfterPlacement(void) override;
        virtual void control(void) override;
        virtual void movement(void) override;
        virtual void makeActorAlive(void) override;
        virtual void makeActorDead(void) override;
        virtual void calcAnim(void) override;

        virtual void attackSensor(al::HitSensor*, al::HitSensor*) override;
        virtual bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;

        virtual const char* getName() const override {
            if (mInfo)
                return mInfo->puppetName;
            return mActorName;
        }

        void initOnline(PuppetInfo* pupInfo);

        void startAction(const char* actName);
        void hairControl();

        void setBlendWeight(int index, float weight) { al::setSklAnimBlendWeight(getCurrentModel(), weight, index); };

        bool isNeedBlending();

        bool isInCaptureList(const char* hackName);

        PuppetInfo* getInfo() { return mInfo; }

        bool addCapture(PuppetHackActor* capture, const char* hackType);

        al::LiveActor* getCurrentModel();

        int getMaxCaptures() {return mCaptures->getEntryCount(); };

        void debugTeleportCaptures(const sead::Vector3f& pos);

        void debugTeleportCapture(const sead::Vector3f& pos, int index);

        void emitJoinEffect();

        bool mIsDebug = false;

    private:
        void changeModel(const char* newModel);

        bool setCapture(const char* captureName);

        void syncPose();

        PlayerCostumeInfo* mCostumeInfo = nullptr;
        PuppetInfo*        mInfo        = nullptr;
        PuppetCapActor*    mPuppetCap   = nullptr;
        PlayerModelHolder* mModelHolder = nullptr;
        HackModelHolder*   mCaptures    = nullptr;
        NameTag*           mNameTag     = nullptr;

        CaptureTypes::Type mCurCapture = CaptureTypes::Type::Unknown;

        bool mIs2DModel = false;

        bool mIsCaptureModel = false;

        float mClosingSpeed = 0;

        FreezePlayerBlock* mFreezeTagIceBlock = nullptr;
};

PlayerCostumeInfo* initMarioModelPuppet(
    al::LiveActor* player,
    const al::ActorInitInfo& initInfo,
    char const* bodyName,
    char const* capName,
    int subActorNum,
    al::AudioKeeper* audioKeeper
);

PlayerHeadCostumeInfo* initMarioHeadCostumeInfo(
    al::LiveActor* player,
    const al::ActorInitInfo& initInfo,
    const char* headModelName,
    const char* capModelName,
    const char* headType,
    const char* headSuffix
);
