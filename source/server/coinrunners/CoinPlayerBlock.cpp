#include "server/coinrunners/CoinPlayerBlock.h"

#include "al/util.hpp"

CoinPlayerBlock::CoinPlayerBlock(const char* name) : al::LiveActor(name) {}

void CoinPlayerBlock::init(al::ActorInitInfo const& info) {
    al::initActorWithArchiveName(this, info, "CoinPlayerBlock", nullptr);
    al::initNerve(this, &nrvCoinPlayerBlockAppear, 0);

    al::invalidateClipping(this);

    al::setDitherAnimSphereRadius(this, 0.f);

    makeActorDead();
}

void CoinPlayerBlock::initAfterPlacement(void) {
    al::LiveActor::initAfterPlacement();
    return;
}

bool CoinPlayerBlock::receiveMsg(const al::SensorMsg* message, al::HitSensor* source, al::HitSensor* target) {
    return false;
}

void CoinPlayerBlock::attackSensor(al::HitSensor* target, al::HitSensor* source) {}

void CoinPlayerBlock::control(void) {
    al::LiveActor::control();
}

void CoinPlayerBlock::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &nrvCoinPlayerBlockAppear);
    mIsLocked = true;
}

void CoinPlayerBlock::end() {
    al::setNerve(this, &nrvCoinPlayerBlockDisappear);
    mIsLocked = false;
}

void CoinPlayerBlock::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
        al::setScaleAll(this, 1.f);
    }

    mDitheringOffset = -420.f; // Resets the dithering offset to slightly beyond the fully opaque value
    al::setDitherAnimSphereRadius(this, 0.f);

    if (al::isActionEnd(this)) {
        al::setNerve(this, &nrvCoinPlayerBlockWait);
    }
}

void CoinPlayerBlock::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    // Start by updating the lerp on the dithering offset
    mDitheringOffset = al::lerpValue(mDitheringOffset, -65.f, 0.02f);

    // Update dithering based on current camera info
    al::CameraPoser*    curPoser;
    al::CameraDirector* director = mSceneInfo->mCameraDirector;

    // Verify the scene info has a camera director, then set the poser
    if (director) {
        al::CameraPoseUpdater* updater = director->getPoseUpdater(0);
        if (updater && updater->mTicket) {
            curPoser = updater->mTicket->mPoser;
        }
    }

    if (curPoser) { // Actually update the dithering stuff here
        float dist = al::calcDistance(this, curPoser->mPosition);
        al::setDitherAnimSphereRadius(this, dist + mDitheringOffset);
    }
}

void CoinPlayerBlock::exeDisappear() {
    float scale = al::lerpValue(*al::getScaleX(this), 0.f, 0.2f);
    al::setScaleAll(this, scale);

    if (al::isNearZero(scale, 0.05f)) {
        al::setNerve(this, &nrvCoinPlayerBlockDead);
    }
}

void CoinPlayerBlock::exeDead() {
    if (al::isFirstStep(this)) {
        kill();
    }
}

namespace {
    NERVE_IMPL(CoinPlayerBlock, Appear)
    NERVE_IMPL(CoinPlayerBlock, Wait)
    NERVE_IMPL(CoinPlayerBlock, Disappear)
    NERVE_IMPL(CoinPlayerBlock, Dead)
}
