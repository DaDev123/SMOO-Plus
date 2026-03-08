#include "Scene/Twists/Fludd/actors/FluddTurbo.hpp"

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

namespace ca {

namespace {
NERVE_IMPL(FluddTurbo, Wait)
NERVES_MAKE_STRUCT(FluddTurbo, Wait)
}  // namespace

FluddTurbo::FluddTurbo(const char* name) : al::LiveActor(name) {}

void FluddTurbo::init(al::ActorInitInfo const& info) {
    al::initActorWithArchiveName(this, info, "FluddTurbo", nullptr);
    al::initNerve(this, &NrvFluddTurbo.Wait, 1);
    mtxConnector = al::createMtxConnector(this);
    this->makeActorAlive();
    al::hideModel(this);
}

void FluddTurbo::connect(al::LiveActor* f) {
    if (!al::isMtxConnectorConnecting(mtxConnector) || al::calcDistance(this, f) > 50.0f) {
        al::setTrans(this, al::getTrans(f));
        al::attachMtxConnectorToJoint(mtxConnector, f, "nozzle_center");
    }
}

void FluddTurbo::activate(bool startEffect) {
    al::tryStartActionIfNotPlaying(this, "OldShoot");
    if (al::isSklAnimEnd(this, 0))
        al::setSklAnimFrame(this, 0, 0);
    if (startEffect) {
        if (effectDelay >= 1) {
            al::tryEmitEffect(this, "WaterBall", al::getTransPtr(this));
            effectDelay = 0;
        } else {
            effectDelay++;
        }
        al::startSe(this, "Attack");
    }
}

void FluddTurbo::deactivate() {
    al::tryDeleteEffect(this, "WaterBall");
    al::setSklAnimFrame(this, 0, 0);
}

void FluddTurbo::listenAppear() {
    this->appear();
}

bool FluddTurbo::receiveMsg(const al::SensorMsg* message, al::HitSensor* source, al::HitSensor* target) {
    return false;
}

void FluddTurbo::attackSensor(al::HitSensor* source, al::HitSensor* target) {}

void FluddTurbo::control() {
    al::connectPoseQT(this, mtxConnector, *quat, *posOffset);
    isConnecting = al::isMtxConnectorConnecting(mtxConnector);
}

void FluddTurbo::exeWait() {}

}  // namespace ca