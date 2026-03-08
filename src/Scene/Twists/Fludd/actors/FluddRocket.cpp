#include "Scene/Twists/Fludd/actors/FluddRocket.hpp"

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
NERVE_IMPL(FluddRocket, Wait)
NERVES_MAKE_STRUCT(FluddRocket, Wait)
}  // namespace

FluddRocket::FluddRocket(const char* name) : al::LiveActor(name) {}

void FluddRocket::init(al::ActorInitInfo const& info) {
    al::initActorWithArchiveName(this, info, "FluddRocket", nullptr);
    al::initNerve(this, &NrvFluddRocket.Wait, 1);
    mtxConnector = al::createMtxConnector(this);
    this->makeActorAlive();
    al::hideModel(this);
}

void FluddRocket::connect(al::LiveActor* f) {
    if (!al::isMtxConnectorConnecting(mtxConnector) || al::calcDistance(this, f) > 50.0f) {
        al::setTrans(this, al::getTrans(f));
        al::attachMtxConnectorToJoint(mtxConnector, f, "nozzle_center");
    }
}

void FluddRocket::activate(bool startEffect) {
    al::tryStartActionIfNotPlaying(this, "Shoot");
    if (al::isSklAnimEnd(this, 0))
        al::setSklAnimFrame(this, 0, 0);
    if (startEffect) {
        al::tryEmitEffect(this, "WaterRoadMove", al::getTransPtr(this));
        al::startSe(this, "Launch");
    }
}

void FluddRocket::deactivate() {
    al::tryDeleteEffect(this, "WaterRoadMove");
    al::setSklAnimFrame(this, 0, 0);
}

void FluddRocket::listenAppear() {
    this->appear();
}

bool FluddRocket::receiveMsg(const al::SensorMsg* message, al::HitSensor* source, al::HitSensor* target) {
    return false;
}

void FluddRocket::attackSensor(al::HitSensor* source, al::HitSensor* target) {}

void FluddRocket::control() {
    al::connectPoseQT(this, mtxConnector, *quat, *posOffset);
    isConnecting = al::isMtxConnectorConnecting(mtxConnector);
}

void FluddRocket::exeWait() {}

}  // namespace ca