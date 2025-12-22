#include "server/freeze/FreezePlayerBlock.h"
#include "Library/Camera/CameraTicket.h"
#include "Library/LiveActor/ActorSceneInfo.h"
#include "al/Library/Camera/CameraDirector.h"
#include "al/Library/Camera/CameraPoseUpdater.h"
#include "al/Library/Camera/CameraPoser.h"
#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorClippingFunction.h"
#include "al/Library/LiveActor/ActorInitUtil.h"
#include "al/Library/LiveActor/ActorModelFunction.h"
#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/Math/MathUtil.h"
#include "al/Library/Nerve/NerveUtil.h"

FreezePlayerBlock::FreezePlayerBlock(const char* name) : al::LiveActor(name) {}

void FreezePlayerBlock::init(al::ActorInitInfo const& info) {
    al::initActorWithArchiveName(this, info, "FreezePlayerBlock", nullptr);
    al::initNerve(this, &NrvFreezePlayerBlock.Appear, 0);

    al::invalidateClipping(this);

    al::setDitherAnimSphereRadius(this, 0.f);

    makeActorDead();
}

void FreezePlayerBlock::initAfterPlacement(void) {
    al::LiveActor::initAfterPlacement();
    return;
}

bool FreezePlayerBlock::receiveMsg(const al::SensorMsg* message, al::HitSensor* source,
                                   al::HitSensor* target) {
    return false;
}

void FreezePlayerBlock::attackSensor(al::HitSensor* target, al::HitSensor* source) {
    return;
}

void FreezePlayerBlock::control(void) {
    al::LiveActor::control();
}

void FreezePlayerBlock::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvFreezePlayerBlock.Appear);
    mIsLocked = true;
}

void FreezePlayerBlock::end() {
    al::setNerve(this, &NrvFreezePlayerBlock.Disappear);
    mIsLocked = false;
}

void FreezePlayerBlock::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
        al::setScaleAll(this, 1.f);
    }

    mDitheringOffset =
        -420.f;  // Resets the dithering offset to slightly beyond the fully opaque value
    al::setDitherAnimSphereRadius(this, 0.f);

    if (al::isActionEnd(this))
        al::setNerve(this, &NrvFreezePlayerBlock.Wait);
}

void FreezePlayerBlock::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");

    // Start by updating the lerp on the dithering offset
    mDitheringOffset = al::lerpValue(mDitheringOffset, -65.f, 0.02f);

    // Update dithering based on current camera info
    al::CameraPoser* curPoser;
    al::CameraDirector* director = mSceneInfo->cameraDirector;

    // Verify the scene info has a camera director, then set the poser
    if (director) {
        al::CameraPoseUpdater* updater = director->getPoseUpdater(0);
        if (updater && updater->mTicket)
            curPoser = updater->mTicket->mPoser;
    }

    if (curPoser) {  // Actually update the dithering stuff here
        float dist = al::calcDistance(this, curPoser->mPosition);
        al::setDitherAnimSphereRadius(this, dist + mDitheringOffset);
    }
}

void FreezePlayerBlock::exeDisappear() {
    float scale = al::lerpValue(al::getScaleX(this), 0.f, 0.2f);
    al::setScaleAll(this, scale);

    if (al::isNearZero(scale, 0.05f))
        al::setNerve(this, &NrvFreezePlayerBlock.Dead);
}

void FreezePlayerBlock::exeDead() {
    if (al::isFirstStep(this))
        kill();
}
