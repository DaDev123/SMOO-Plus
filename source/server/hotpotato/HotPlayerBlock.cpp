#include "server/hotpotato/HotPlayerBlock.h"
#include "al/LiveActor/LiveActor.h"
#include "al/util.hpp"
#include "al/util/LiveActorUtil.h"
#include "al/util/MathUtil.h"
#include "al/util/NerveUtil.h"
#include "math/seadVector.h"
#include "rs/util.hpp"

HotPlayerBlock::HotPlayerBlock(const char* name)
    : al::LiveActor(name)
{
}

void HotPlayerBlock::init(al::ActorInitInfo const& info)
{
    al::initActorWithArchiveName(this, info, "HotPlayerBlock", nullptr);
    al::initNerve(this, &nrvHotPlayerBlockAppear, 0);

    al::invalidateClipping(this);

    al::setDitherAnimSphereRadius(this, 0.f);

    makeActorDead();
}

void HotPlayerBlock::initAfterPlacement(void)
{
    al::LiveActor::initAfterPlacement();
    return;
}

bool HotPlayerBlock::receiveMsg(const al::SensorMsg* message, al::HitSensor* source, al::HitSensor* target)
{
    return false;
}

void HotPlayerBlock::attackSensor(al::HitSensor* target, al::HitSensor* source)
{
    return;
}

void HotPlayerBlock::control(void)
{
    al::LiveActor::control();
}

void HotPlayerBlock::appear()
{
    al::LiveActor::appear();
    al::setNerve(this, &nrvHotPlayerBlockAppear);
    mIsLocked = true;
}

void HotPlayerBlock::end()
{
    al::setNerve(this, &nrvHotPlayerBlockDisappear);
    mIsLocked = false;
}

void HotPlayerBlock::exeAppear()
{
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
        al::setScaleAll(this, 1.f);
    }

    mDitheringOffset = -420.f; // Resets the dithering offset to slightly beyond the fully opaque value
    al::setDitherAnimSphereRadius(this, 0.f);

    if (al::isActionEnd(this))
        al::setNerve(this, &nrvHotPlayerBlockWait);
}

void HotPlayerBlock::exeWait()
{
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");

    // Start by updating the lerp on the dithering offset
    mDitheringOffset = al::lerpValue(mDitheringOffset, -65.f, 0.02f);

    // Update dithering based on current camera info
    al::CameraPoser* curPoser;
    al::CameraDirector* director = mSceneInfo->mCameraDirector;

    // Verify the scene info has a camera director, then set the poser
    if (director) {
        al::CameraPoseUpdater* updater = director->getPoseUpdater(0);
        if (updater && updater->mTicket)
            curPoser = updater->mTicket->mPoser;
    }

    if (curPoser) { // Actually update the dithering stuff here
        float dist = al::calcDistance(this, curPoser->mPosition);
        al::setDitherAnimSphereRadius(this, dist + mDitheringOffset);
    }
}

void HotPlayerBlock::exeDisappear()
{
    float scale = al::lerpValue(*al::getScaleX(this), 0.f, 0.2f);
    al::setScaleAll(this, scale);

    if (al::isNearZero(scale, 0.05f))
        al::setNerve(this, &nrvHotPlayerBlockDead);
}

void HotPlayerBlock::exeDead()
{
    if (al::isFirstStep(this))
        kill();
}

namespace {
NERVE_IMPL(HotPlayerBlock, Appear)
NERVE_IMPL(HotPlayerBlock, Wait)
NERVE_IMPL(HotPlayerBlock, Disappear)
NERVE_IMPL(HotPlayerBlock, Dead)
} // namespace