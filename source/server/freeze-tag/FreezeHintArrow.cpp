#include "server/freeze-tag/FreezeHintArrow.h"

#include "al/util/VectorUtil.h"

#include "sead/math/seadQuat.h"

#include "server/gamemode/GameMode.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/freeze-tag/FreezeTagInfo.h"

FreezeHintArrow::FreezeHintArrow(const char* name) : al::LiveActor(name) {}

void FreezeHintArrow::init(al::ActorInitInfo const& info) {
    al::initActorWithArchiveName(this, info, "FreezeHintArrow", nullptr);
    al::initNerve(this, &nrvFreezeHintArrowWait, 0);
    al::invalidateClipping(this);

    makeActorAlive();
}

void FreezeHintArrow::initAfterPlacement(void) {
    al::LiveActor::initAfterPlacement();

    mPlayer      = al::tryGetPlayerActor(mSceneInfo->mPlayerHolder, 0);
    mTargetTrans = al::getTransPtr(mPlayer);
    mArrowTrans  = al::getTransPtr(this);

    if (!GameModeManager::instance()->isMode(GameMode::FREEZETAG)) {
        return;
    }

    mInfo = GameModeManager::instance()->getInfo<FreezeTagInfo>();
}

bool FreezeHintArrow::receiveMsg(const al::SensorMsg* message, al::HitSensor* source, al::HitSensor* target) {
    return false;
}

void FreezeHintArrow::attackSensor(al::HitSensor* target, al::HitSensor* source) {}

void FreezeHintArrow::control(void) {
    al::LiveActor::control();
}

void FreezeHintArrow::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &nrvFreezeHintArrowWait);
}

void FreezeHintArrow::end() {
    kill();
}

void FreezeHintArrow::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    if (   !GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG) // we're not in freeze-tag mode
        || mInfo->isPlayerRunner()                                            // or we're a runner
        || !mInfo->isRound()                                                  // or we're outside of a round
        || !mTargetTrans                                                      // or there is no runner far enough away
        || mPlayer->getPlayerHackKeeper()->currentHackActor                   // or the target hides in a capture
    ) {
        // hide the arrow
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

    if (!mIsActive) {
        return;
    }

    // Check distance and set the visiblity based on that
    mVisibilityCooldown = al::clamp(mVisibilityCooldown - 1, 0, 255); // Decrease visiblity cooldown, capped at zero

    if (mVisibilityCooldown == 0) {
        mDistanceSq = vecDistanceSq(al::getTrans(this), *mTargetTrans);
        mIsVisible  = mMinDistanceSq < mDistanceSq;

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

namespace {
    NERVE_IMPL(FreezeHintArrow, Wait)
}
