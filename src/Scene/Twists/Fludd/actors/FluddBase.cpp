#include "Scene/Twists/Fludd/actors/FluddBase.hpp"

#include "al/Library/Collision/PartsConnectorUtil.h"
#include "al/Library/Effect/EffectSystemInfo.h"
#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorAnimFunction.h"
#include "al/Library/LiveActor/ActorClippingFunction.h"
#include "al/Library/LiveActor/ActorFlagFunction.h"
#include "al/Library/LiveActor/ActorInitFunction.h"
#include "al/Library/LiveActor/ActorInitUtil.h"
#include "al/Library/LiveActor/ActorModelFunction.h"
#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/LiveActor/ActorSensorUtil.h"
#include "al/Library/Nerve/NerveSetupUtil.h"
#include "al/Library/Player/PlayerUtil.h"
#include "al/Library/Se/SeFunction.h"

namespace {
NERVE_IMPL(FluddBase, Wait)
NERVES_MAKE_STRUCT(FluddBase, Wait)
}  // namespace

FluddBase::FluddBase(const char* name) : al::LiveActor(name) {}

void FluddBase::init(al::ActorInitInfo const& info) {
    al::initActorWithArchiveName(this, info, "FluddBase", nullptr);
    al::initNerve(this, &NrvFluddBase.Wait, 0);
    mtxConnector = al::createMtxConnector(this);
    this->makeActorAlive();
}

void FluddBase::connect(al::LiveActor* m) {
    if (!al::isMtxConnectorConnecting(mtxConnector) || al::calcDistance(this, m) > 50.0f) {
        al::setTrans(this, al::getTrans(m));
        al::attachMtxConnectorToJoint(mtxConnector, m, "Spine1");
    }
}

void FluddBase::activate() {
    al::tryStartActionIfNotPlaying(this, "Shoot");
    if (al::isSklAnimEnd(this, 0))
        al::setSklAnimFrame(this, 0, 0);
}

void FluddBase::deactivate() {
    al::setSklAnimFrame(this, 0, 0);
}

void FluddBase::listenAppear() {
    this->appear();
}

bool FluddBase::receiveMsg(const al::SensorMsg* message, al::HitSensor* source, al::HitSensor* target) {
    return false;
}

void FluddBase::attackSensor(al::HitSensor* source, al::HitSensor* target) {}

void FluddBase::control() {
    al::connectPoseQT(this, mtxConnector, *quat, *posOffset);
    isConnecting = al::isMtxConnectorConnecting(mtxConnector);
}

void FluddBase::exeWait() {}