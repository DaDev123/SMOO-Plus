#include "actors/PuppetCapActor.h"

#include "al/Library/HitSensor/HitSensorKeeper.h"
#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorAnimFunction.h"
#include "al/Library/LiveActor/ActorInitFunction.h"
#include "al/Library/LiveActor/ActorModelFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/LiveActor/ActorSensorUtil.h"

#include "game/Player/PlayerFunction.h"
#include "game/Util/SensorMsgFunction.h"

#include "helpers.hpp"
#include "Library/Math/MathUtil.h"
#include "math/seadQuat.h"
#include "math/seadVectorFwd.h"
#include "Project/HitSensor/HitSensor.h"
#include "Scene/StageSceneStateModConfig.hpp"
#include "server/Client.hpp"

PuppetCapActor::PuppetCapActor(const char* name) : al::LiveActor(name) {}

void PuppetCapActor::init(const al::ActorInitInfo& initInfo) {
    sead::FixedSafeString<0x20> capModelName;

    PlayerFunction::createCapModelName(&capModelName, tryGetPuppetCapName(mInfo));
    Logger::log("create cap model name\n");
    PlayerFunction::initCapModelActorDemo(this, initInfo, capModelName.cstr());
    Logger::log("init cap model actor demo\n");

    initHitSensor(2);
    Logger::log("init hit sensor\n");
    al::addHitSensor(this, initInfo, "Push", (u32)al::HitSensorType::MapObjSimple, 60.0f, 8,
                     sead::Vector3f::zero);
    Logger::log("push sensor\n");
    al::addHitSensor(this, initInfo, "Attack", (u32)al::HitSensorType::EnemyAttack, 300.0f, 8,
                     sead::Vector3f::zero);
    Logger::log("attack sensor\n");

    al::hideSilhouetteModelIfShow(this);
    Logger::log("hide silhouette\n");
    al::initExecutorModelUpdate(this, initInfo);
    Logger::log("init executor\n");

    mJointKeeper = new (Client::instance()->mHakkunSceneHeap) HackCapJointControlKeeper();
    Logger::log("hack cap join control keeper\n");
    mJointKeeper->initCapJointControl(this);
    Logger::log("init cap joint control\n");

    makeActorDead();
    Logger::log("make actor dead\n");
}

void PuppetCapActor::initAfterPlacement() {
    al::LiveActor::initAfterPlacement();
}

void PuppetCapActor::initOnline(PuppetInfo* pupInfo) {
    mInfo = pupInfo;
}

void PuppetCapActor::movement() {
    al::LiveActor::movement();
}

void PuppetCapActor::control() {
    if (strcmp(mInfo->capAnim, "") != 0) {
        startAction(mInfo->capAnim);
    }

    if (!StageSceneStateModConfig::isLowLatencyEnabled()) {
        sead::Vector3f* trans = al::getTransPtr(this);
        al::lerpVec(trans, *trans, mInfo->capPos, 0.45);
    } else {
        al::setTrans(this, mInfo->capPos);
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