#include "server/hotpotato/HotHintArrow.h"
#include "al/LiveActor/LiveActor.h"
#include "al/util.hpp"
#include "al/util/LiveActorUtil.h"
#include "al/util/MathUtil.h"
#include "al/util/NerveUtil.h"
#include "al/util/VectorUtil.h"
#include "logger.hpp"
#include "math/seadQuat.h"
#include "math/seadVector.h"
#include "rs/util.hpp"
#include "server/Client.hpp"
#include "server/gamemode/GameModeManager.hpp"

HotHintArrow::HotHintArrow(const char* name)
    : al::LiveActor(name)
{
}

void HotHintArrow::init(al::ActorInitInfo const& info)
{
    al::initActorWithArchiveName(this, info, "HotHintArrow", nullptr);
    al::initNerve(this, &nrvHotHintArrowWait, 0);
    al::invalidateClipping(this);

    makeActorAlive();
}

void HotHintArrow::initAfterPlacement(void)
{
    al::LiveActor::initAfterPlacement();

    mPlayer = al::tryGetPlayerActor(mSceneInfo->mPlayerHolder, 0);
    mTargetTrans = al::getTransPtr(mPlayer);
    mArrowTrans = al::getTransPtr(this);

    if(!GameModeManager::instance()->isMode(GameMode::HOTPOTATO))
        return;

    mInfo = GameModeManager::instance()->getInfo<HotPotatoInfo>();

    return;
}

bool HotHintArrow::receiveMsg(const al::SensorMsg* message, al::HitSensor* source, al::HitSensor* target)
{
    return false;
}

void HotHintArrow::attackSensor(al::HitSensor* target, al::HitSensor* source)
{
    return;
}

void HotHintArrow::control(void)
{
    al::LiveActor::control();
}

void HotHintArrow::appear()
{
    al::LiveActor::appear();
    al::setNerve(this, &nrvHotHintArrowWait);
}

void HotHintArrow::end()
{
    kill();
}

void HotHintArrow::exeWait()
{
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");
    
    bool isInHotMode = GameModeManager::instance()->isModeAndActive(GameMode::HOTPOTATO);
    if(!isInHotMode || mInfo->mIsPlayerRunner || !mInfo->mIsRound || !mTargetTrans || mPlayer->getPlayerHackKeeper()->currentHackActor) {
        mSize = al::lerpValue(mSize, 0.f, 0.3f);
        al::setScaleAll(this, mSize);
        return;
    }

    // Update translation to player
    *mArrowTrans = al::getTrans(mPlayer);
    mArrowTrans->y += 200.f;

    // Update size based on active & visible bools
    mSize = al::lerpValue(mSize, mIsActive && mIsVisible ? 1.f : 0.f, 0.2f);
    al::setScaleAll(this, mSize);

    if(!mIsActive)
        return;

    // Check distance and set the visiblity based on that
    mVisibilityCooldown = al::clamp(mVisibilityCooldown - 1, 0, 255); // Decrease visiblity cooldown, capped at zero

    if(mVisibilityCooldown == 0) {
        mDistance = al::calcDistance(this, *mTargetTrans);
        mIsVisible = mDistance > mMinDistance;

        if(mIsVisible != mWasVisible) {
            mWasVisible = mIsVisible;
            mVisibilityCooldown = 150;
        }
    }

    // If the actor is visible, update the rotation based on the target location
    if(mIsVisible) {
        sead::Vector3f direction = *mTargetTrans - *mArrowTrans;
        al::normalize(&direction);

        sead::Quatf newQuat;
        al::makeQuatUpFront(&newQuat, mActorUp, direction);

        al::setQuat(this, newQuat);
    }
}

namespace {
NERVE_IMPL(HotHintArrow, Wait)
} // namespace