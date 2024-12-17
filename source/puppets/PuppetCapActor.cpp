#include "actors/PuppetCapActor.h"

#include "al/util.hpp"
#include "al/util/MathUtil.h"
#include "al/util/SensorUtil.h"

#include "game/Player/PlayerFunction.h"

#include "rs/util/SensorUtil.h"

#include "sead/math/seadVector.h"

#include "server/gamemode/GameModeManager.hpp"

PuppetCapActor::PuppetCapActor(const char* name) : al::LiveActor(name) {}

void PuppetCapActor::init(al::ActorInitInfo const& initInfo) {
    sead::FixedSafeString<0x20> capModelName;

    PlayerFunction::createCapModelName(&capModelName, tryGetPuppetCapName(mInfo));

    PlayerFunction::initCapModelActorDemo(this, initInfo, capModelName.cstr());

    initHitSensor(2);

    al::addHitSensor(this, initInfo, "Push", SensorType::MapObjSimple, 60.0f, 8, sead::Vector3f::zero);

    al::addHitSensor(this, initInfo, "Attack", SensorType::EnemyAttack, 300.0f, 8, sead::Vector3f::zero);

    al::hideSilhouetteModelIfShow(this);

    al::initExecutorModelUpdate(this, initInfo);

    mJointKeeper = new HackCapJointControlKeeper();

    mJointKeeper->initCapJointControl(this);

    makeActorDead();
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
    if (mInfo->capAnim) {
        startAction(mInfo->capAnim);
    }

    sead::Vector3f* cPos = al::getTransPtr(this);

    if (*cPos != mInfo->capPos) {
        al::lerpVec(cPos, *cPos, mInfo->capPos, 0.45);
    }

    mJointKeeper->mJointRot.x = al::lerpValue(mJointKeeper->mJointRot.x, mInfo->capRot.x, 0.85);
    mJointKeeper->mJointRot.y = al::lerpValue(mJointKeeper->mJointRot.y, mInfo->capRot.y, 0.85);
    mJointKeeper->mJointRot.z = al::lerpValue(mJointKeeper->mJointRot.z, mInfo->capRot.z, 0.85);
    mJointKeeper->mSkew = al::lerpValue(mJointKeeper->mSkew, mInfo->capRot.w, 0.85);
}

void PuppetCapActor::update() {
    al::LiveActor::calcAnim();
    al::LiveActor::movement();
}

void PuppetCapActor::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (!GameModeManager::hasCappyCollision()) {
        return;
    }

    if (al::isSensorPlayer(receiver) && al::isSensorName(sender, "Push")) {
        rs::sendMsgPushToPlayer(receiver, sender);
    }
}

bool PuppetCapActor::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (!GameModeManager::hasCappyBounce()) {
        return false;
    }

    if (al::isMsgPlayerDisregard(msg)) {
        return true;
    }

    if (rs::isMsgPlayerCapTouchJump(msg)) {
        return true;
    }

    if (rs::isMsgPlayerCapTrample(msg)) {
        rs::requestHitReactionToAttacker(msg, receiver, *al::getSensorPos(sender));
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
