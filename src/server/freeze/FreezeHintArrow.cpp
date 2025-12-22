#include "server/freeze/FreezeHintArrow.h"

#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorClippingFunction.h"
#include "al/Library/LiveActor/ActorInitUtil.h"
#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/Math/MathUtil.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "al/Library/Player/PlayerUtil.h"

#include "game/Player/PlayerHackKeeper.h"

#include "math/seadQuat.h"
#include "math/seadVector.h"
#include "server/gamemode/GameModeManager.hpp"

FreezeHintArrow::FreezeHintArrow(const char* name) : al::LiveActor(name) {}

void FreezeHintArrow::init(al::ActorInitInfo const& info) {
    al::initActorWithArchiveName(this, info, "FreezeHintArrow", nullptr);
    al::initNerve(this, &NrvFreezeHintArrow.Wait, 0);
    al::invalidateClipping(this);

    makeActorAlive();
}

void FreezeHintArrow::initAfterPlacement(void) {
    al::LiveActor::initAfterPlacement();

    mPlayer = (PlayerActorBase*)al::tryGetPlayerActor(mSceneInfo->playerHolder, 0);
    mTargetTrans = al::getTransPtr(mPlayer);
    mArrowTrans = al::getTransPtr(this);

    if (!GameModeManager::instance()->isMode(GameMode::FREEZETAG))
        return;

    mInfo = GameModeManager::instance()->getInfo<FreezeTagInfo>();

    return;
}

bool FreezeHintArrow::receiveMsg(const al::SensorMsg* message, al::HitSensor* source, al::HitSensor* target) {
    return false;
}

void FreezeHintArrow::attackSensor(al::HitSensor* target, al::HitSensor* source) {
    return;
}

void FreezeHintArrow::control(void) {
    al::LiveActor::control();
}

void FreezeHintArrow::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvFreezeHintArrow.Wait);
}

void FreezeHintArrow::end() {
    kill();
}

void FreezeHintArrow::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");

    bool isInFreezeMode = GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG);
    if (!isInFreezeMode || mInfo->mIsPlayerRunner || !mInfo->mIsRound || !mTargetTrans || mPlayer->getPlayerHackKeeper()->mCurrentHackActor) {
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

    if (!mIsActive)
        return;

    // Check distance and set the visiblity based on that
    mVisibilityCooldown = al::clamp(mVisibilityCooldown - 1, 0, 255);  // Decrease visiblity cooldown, capped at zero

    if (mVisibilityCooldown == 0) {
        mDistance = al::calcDistance(this, *mTargetTrans);
        mIsVisible = mDistance > mMinDistance;

        if (mIsVisible != mWasVisible) {
            mWasVisible = mIsVisible;
            mVisibilityCooldown = 150;
        }
    }

    // If the actor is visible, update the rotation based on the target location
    if (mIsVisible) {
        sead::Vector3f direction = *mTargetTrans - *mArrowTrans;
        al::normalize(&direction);

        sead::Quatf newQuat;
        al::makeQuatUpFront(&newQuat, mActorUp, direction);

        al::setQuat(this, newQuat);
    }
}
