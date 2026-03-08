#include "Scene/Twists/Fludd/actors/FluddHover.hpp"

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
NERVE_IMPL(FluddHover, Wait)
NERVES_MAKE_STRUCT(FluddHover, Wait)
}  // namespace

FluddHover::FluddHover(const char* name) : al::LiveActor(name) {}

void FluddHover::init(al::ActorInitInfo const& info) {
    al::initActorWithArchiveName(this, info, "FluddHover", nullptr);
    al::initNerve(this, &NrvFluddHover.Wait, 1);
    mtxConnector = al::createMtxConnector(this);
    this->makeActorAlive();
    al::hideModel(this);
}

void FluddHover::connect(al::LiveActor* f) {
    if (!al::isMtxConnectorConnecting(mtxConnector) || al::calcDistance(this, f) > 50.0f) {
        al::setTrans(this, al::getTrans(f));
        al::attachMtxConnectorToJoint(mtxConnector, f, "nozzle_center");
    }
}

void FluddHover::activate(bool startEffect) {
    al::tryStartActionIfNotPlaying(this, "Shoot");
    if (al::isSklAnimEnd(this, 0))
        al::setSklAnimFrame(this, 0, 0);
    if (startEffect)
        al::startSe(this, "WaterRoadMove_loop");
}

void FluddHover::deactivate() {
    al::stopSe(this, "WaterRoadMove_loop", 0, nullptr);
    al::setSklAnimFrame(this, 0, 0);
}

void FluddHover::listenAppear() {
    this->appear();
}

bool FluddHover::receiveMsg(const al::SensorMsg* message, al::HitSensor* source, al::HitSensor* target) {
    return false;
}

void FluddHover::attackSensor(al::HitSensor* source, al::HitSensor* target) {}

void FluddHover::control() {
    al::connectPoseQT(this, mtxConnector, *quat, *posOffset);
    isConnecting = al::isMtxConnectorConnecting(mtxConnector);
}

void FluddHover::exeWait() {}

}  // namespace ca