#include "actors/PuppetCapActor.h"

#include "sead/heap/seadHeapMgr.h"
#include "sead/math/seadQuat.h"
#include "sead/math/seadVectorFwd.h"

#include "al/Library/HitSensor/HitSensorKeeper.h"
#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorAnimFunction.h"
#include "al/Library/LiveActor/ActorInitFunction.h"
#include "al/Library/LiveActor/ActorModelFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/LiveActor/ActorSensorUtil.h"
#include "al/Library/Math/MathUtil.h"
#include "al/Library/Memory/HeapUtil.h"
#include "al/Project/HitSensor/HitSensor.h"

#include "game/Player/PlayerFunction.h"
#include "game/Util/SensorMsgFunction.h"

#include "helpers.hpp"
#include "Scene/StageSceneStateModConfig.hpp"

PuppetCapActor::PuppetCapActor(const char* name) : al::LiveActor(name) {}

void PuppetCapActor::init(const al::ActorInitInfo& initInfo) {
    sead::ScopedCurrentHeapSetter setter(al::getSceneHeap());

    sead::FixedSafeString<0x20> capModelName;

    PlayerFunction::createCapModelName(&capModelName, tryGetPuppetCapName(mInfo));
    PlayerFunction::initCapModelActorDemo(this, initInfo, capModelName.cstr());

    initHitSensor(2);
    al::addHitSensor(this, initInfo, "Push", (u32)al::HitSensorType::MapObjSimple, 60.0f, 8,
                     sead::Vector3f::zero);
    al::addHitSensor(this, initInfo, "Attack", (u32)al::HitSensorType::EnemyAttack, 300.0f, 8,
                     sead::Vector3f::zero);

    al::hideSilhouetteModelIfShow(this);
    al::initExecutorModelUpdate(this, initInfo);

    mJointKeeper = new HackCapJointControlKeeper();
    mJointKeeper->initCapJointControl(this);

    makeActorDead();
}

void PuppetCapActor::initOnline(PuppetInfo* pupInfo) {
    mInfo = pupInfo;
}

void PuppetCapActor::control() {
    if (!mInfo->capAnim.isEmpty()) {
        startAction(mInfo->capAnim.cstr());
    }

    if (StageSceneStateModConfig::isLowLatencyEnabled()) {
        al::setTrans(this, mInfo->capPos);
    } else {
        sead::Vector3f* trans = al::getTransPtr(this);
        al::lerpVec(trans, *trans, mInfo->capPos, 0.45);
    }
    al::setQuat(this, mInfo->capQuat);

    mJointKeeper->mJointRot.x = mInfo->capRot.x;
    mJointKeeper->mJointRot.y = mInfo->capRot.y;
    mJointKeeper->mJointRot.z = mInfo->capRot.z;
    mJointKeeper->mSkew = mInfo->capRot.w;
}

void PuppetCapActor::update() {
    al::LiveActor::calcAnim();
    al::LiveActor::movement();
}

void PuppetCapActor::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (!StageSceneStateModConfig::isCapCollisionEnabled()) {
        return;
    }

    if (al::isSensorPlayer(receiver) && al::isSensorName(sender, "Push")) {
        rs::sendMsgPushToPlayer(receiver, sender);
    }
}

bool PuppetCapActor::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (!StageSceneStateModConfig::isCapBounceEnabled()) {
        return false;
    }

    if (al::isMsgPlayerDisregard(msg)) {
        return true;
    }

    if (rs::isMsgPlayerCapTouchJump(msg)) {
        return true;
    }

    if (rs::isMsgPlayerCapTrample(msg)) {
        rs::requestHitReactionToAttacker(msg, receiver, al::getSensorPos(sender));
        return true;
    }

    return false;
}

void PuppetCapActor::startAction(const char* actName) {
    if (al::tryStartActionIfNotPlaying(this, actName)) {
        const char* curActName = al::getActionName(this);
        if (curActName) {
            if (al::isSklAnimExist(this, curActName)) {
                al::clearSklAnimInterpole(this);
            }
        }
    }
}