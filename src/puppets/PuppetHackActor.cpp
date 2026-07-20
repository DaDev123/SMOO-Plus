#include "actors/PuppetHackActor.h"

#include "sead/prim/seadSafeString.h"

#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorAnimFunction.h"
#include "al/Library/LiveActor/ActorClippingFunction.h"
#include "al/Library/LiveActor/ActorCollisionFunction.h"
#include "al/Library/LiveActor/ActorFlagFunction.h"
#include "al/Library/LiveActor/ActorInitUtil.h"
#include "al/Library/LiveActor/ActorModelFunction.h"
#include "al/Library/LiveActor/ActorSensorUtil.h"

PuppetHackActor::PuppetHackActor(const char* name) : al::LiveActor(name) {}

void PuppetHackActor::init(al::ActorInitInfo const& initInfo) {
    // hk::diag::logLine("Creating Hack Puppet: %s", mHackType.cstr());

    al::initActorWithArchiveName(this, initInfo, mHackType, nullptr);

    al::hideSilhouetteModelIfShow(this);

    if (al::isExistDitherAnimator(this)) {
        // hk::diag::logLine("Disabling Dither Animator.");
        al::invalidateDitherAnim(this);
    }

    if (al::isExistCollisionParts(this)) {
        // hk::diag::logLine("Disabling Collision.");
        al::invalidateCollisionParts(this);
    }

    al::invalidateHitSensors(this);

    al::setClippingInfo(this, 999999999.0f, 0);
    al::setClippingNearDistance(this, 999999999.0f);
    al::validateClipping(this);

    al::offCollide(this);

    makeActorDead();

    startHackAnim(true);  // this hack puppet will always be captured so its Hack visibility should be true

    startAction("Wait");
}

void PuppetHackActor::initAfterPlacement() {
    al::LiveActor::initAfterPlacement();
}

void PuppetHackActor::initOnline(PuppetInfo* pupInfo, const char* hackType) {
    mInfo = pupInfo;
    mHackType = hackType;
}

void PuppetHackActor::movement() {
    al::LiveActor::movement();
}

void PuppetHackActor::control() {}

void PuppetHackActor::startAction(sead::SafeString actName) {
    if (actName.isEmpty())
        return;

    // Get the currently playing action
    const char* curActName = al::getActionName(this);

    // Check if we need to start a new action or restart the current one
    bool needsStart = false;

    if (!curActName || !al::isEqualString(curActName, actName)) {
        // Different action - definitely need to start it
        needsStart = true;
    } else if (al::isActionEnd(this)) {
        // Same action but it's ended - need to restart for looping
        needsStart = true;
    }

    if (needsStart) {
        // Try to start the action (will restart even if already playing)
        if (al::tryStartAction(this, actName.cstr())) {
            // Clear interpolation for clean animation start
            if (al::isSklAnimExist(this, actName.cstr())) {
                al::clearSklAnimInterpole(this);
            }
        }
    }
}

void PuppetHackActor::startHackAnim(bool isOn) {
    const char* animName = isOn ? "HackOn" : "HackOff";
    const char* capOffName = isOn ? "HackOnCapOff" : "HackOffCapOff";

    if (al::isVisAnimExist(this, animName)) {
        al::startVisAnim(this, animName);
    } else if (al::isVisAnimExist(this, capOffName)) {
        al::startVisAnim(this, capOffName);
    }

    if (al::isMtpAnimExist(this, animName)) {
        al::startMtpAnim(this, animName);
    } else if (al::isMtpAnimExist(this, capOffName)) {
        al::startMtpAnim(this, capOffName);
    }

    if (al::isMclAnimExist(this, animName)) {
        al::startMclAnim(this, animName);
    } else if (al::isMclAnimExist(this, capOffName)) {
        al::startMclAnim(this, capOffName);
    }

    // note: we will need to handle special names for hack start anims
}