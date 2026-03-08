#include "Scene/Twists/Fludd/FluddTwist.hpp"

#include "al/Library/Camera/CameraUtil.h"
#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Effect/EffectSystemInfo.h"
#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorAnimFunction.h"
#include "al/Library/LiveActor/ActorModelFunction.h"
#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/ActorPoseKeeper.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/LiveActor/ActorSensorUtil.h"
#include "al/Library/Math/MathUtil.h"
#include "al/Library/Nature/NatureUtil.h"
#include "al/Library/Player/PlayerUtil.h"
#include "al/Library/Scene/SceneUtil.h"

#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Player/PlayerAnimator.h"
#include "game/Player/PlayerFunction.h"
#include "game/Player/PlayerHackKeeper.h"
#include "game/Player/PlayerInput.h"
#include "game/Player/PlayerModelHolder.h"
#include "game/Util/ActorDimensionKeeper.h"
#include "game/Util/DemoUtil.h"
#include "game/Util/PlayerUtil.h"
#include "game/Util/SensorMsgFunction.h"

#include "rs/util.hpp"

// ============================================================
//  Static member definitions
// ============================================================

bool FluddTwist::sFluddEnabled = false;

StageScene* FluddTwist::sStageScene = nullptr;

FluddBase* FluddTwist::sBase = nullptr;
ca::FluddHover* FluddTwist::sHover = nullptr;
ca::FluddRocket* FluddTwist::sRocket = nullptr;
ca::FluddTurbo* FluddTwist::sTurbo = nullptr;

PlayerActorHakoniwa* FluddTwist::sMario = nullptr;
al::LiveActor* FluddTwist::sMarioModel = nullptr;
bool FluddTwist::sIs2D = false;
bool FluddTwist::sIsHack = false;
bool FluddTwist::sIsPGrounded = false;
bool FluddTwist::sIsPUnderWater = false;
bool FluddTwist::sIsPInWater = false;

int FluddTwist::sFluddMode = 0;
float FluddTwist::sTank = 100.f;
float FluddTwist::sFluddRecharge = 0.4f;
float FluddTwist::sFluddVel = 2.8f;
float FluddTwist::sFluddDischarge = 0.1f;
float FluddTwist::sChargeTimer = 0.f;
float FluddTwist::sChargeTimerDecrease = 1.5f;
float FluddTwist::sTankStopValue = 0.f;
float FluddTwist::sTankRunoutVal = 34.f;
bool FluddTwist::sRecharging = false;
bool FluddTwist::sTStopValueSet = false;
bool FluddTwist::sLJCancel = false;
bool FluddTwist::sStickActive = false;
bool FluddTwist::sSetNrvGrounded = false;
bool FluddTwist::sDoOnce = false;
bool FluddTwist::sIsFirstBoost = true;
int FluddTwist::sDoubleBoostFrames = 0;

// ============================================================
//  Public lifecycle
// ============================================================

void FluddTwist::init(al::ActorInitInfo const& info) {
    if (sBase)  // already initialized, don't recreate actors on stage reload
        return;

    sBase = new FluddBase("Fludd");
    sBase->init(info);

    sHover = new ca::FluddHover("Hover");
    sHover->init(info);

    sRocket = new ca::FluddRocket("Rocket");
    sRocket->init(info);

    sTurbo = new ca::FluddTurbo("Turbo");
    sTurbo->init(info);
}

void FluddTwist::onStageInit(StageScene* scene) {
    sStageScene = scene;
    fluddTankFill();
    sFluddMode = 0;
    sRecharging = false;
    sTStopValueSet = false;
    sDoOnce = true;
    if (sBase) {
        al::hideModel(sBase);
        al::hideModel(sHover);
        al::hideModel(sRocket);
        al::hideModel(sTurbo);
    }
}

void FluddTwist::onStageDeath() {
    fluddTankFill();
    sStageScene = nullptr;
}

void FluddTwist::toggle() {
    sFluddEnabled = !sFluddEnabled;
    if (!sFluddEnabled && sBase) {
        al::hideModel(sBase);
        al::hideModel(sHover);
        al::hideModel(sRocket);
        al::hideModel(sTurbo);
        sDoOnce = true;
    }
}

void FluddTwist::update(PlayerActorHakoniwa* p1) {
    if (!sFluddEnabled || !p1 || !sStageScene)
        return;

    sMario = p1;
    setRefs();

    if (sStickActive && al::isPadTriggerPressLeftStick(-1) && !al::isPadHoldL(-1))
        changeFluddModeL();
    if (al::isPadTriggerLeft(-1))
        changeFluddModeL();
    if (al::isPadTriggerRight(-1))
        changeFluddModeR();
    if (al::isPadHoldL(-1) && al::isPadTriggerPressLeftStick(-1))
        sStickActive = !sStickActive;

    updateModels();

    if (!sTStopValueSet) {
        sTankStopValue = stopTankValue();
        sTStopValueSet = true;
    }

    if (!sIsPGrounded && sFluddMode == 2)
        sSetNrvGrounded = true;

    if (!sIs2D && isTankEmpty(sTankStopValue) && sLJCancel) {
        sLJCancel = false;
        p1->setNerveOnGround();
    }

    if (al::isPadHoldR(-1) && !sRecharging) {
        if (canFluddActivate())
            activateFludd();
    } else {
        deactivateFludd();
    }
}

// ============================================================
//  Public accessors
// ============================================================

int FluddTwist::getFluddMode() {
    return sFluddMode;
}
float FluddTwist::getTank() {
    return sTank;
}
float FluddTwist::getChargeTimer() {
    return sChargeTimer;
}
float FluddTwist::getTankStopValue() {
    return sTankStopValue;
}
bool FluddTwist::isRecharging() {
    return sRecharging;
}
bool FluddTwist::isStickActive() {
    return sStickActive;
}

// ============================================================
//  Private helpers
// ============================================================

void FluddTwist::setRefs() {
    sIsHack = sMario->mHackKeeper->mHackActor != nullptr;
    sMarioModel = sMario->mModelHolder->mCurrentModel->actor;
    sIs2D = sMario->mDimensionKeeper->is2D();
    sIsPGrounded = rs::isPlayerOnGround(sMario);
    sIsPUnderWater = rs::isPlayerInWater(sMario);
    sIsPInWater = al::isInWaterPos(sMario, al::getTrans(sMario));
}

void FluddTwist::firstTimeSetup() {
    sBase->activate();
    setFluddModeValues();
    sHover->activate(false);
    sRocket->activate(false);
    sTurbo->activate(false);
}

void FluddTwist::updateModels() {
    if (sIs2D || sIsHack || rs::isActiveDemo(sMario)) {
        al::hideModel(sBase);
        al::hideModel(sRocket);
        al::hideModel(sHover);
        al::hideModel(sTurbo);
        sDoOnce = true;
    } else {
        if (sDoOnce) {
            al::showModel(sBase);
            firstTimeSetup();
            sDoOnce = false;
        }
        sBase->connect(sMarioModel);
        sHover->connect(sBase);
        sRocket->connect(sBase);
        sTurbo->connect(sBase);
    }
}

void FluddTwist::fluddTankFill() {
    sTank = 100.f;
}

bool FluddTwist::isTankEmpty(float threshold) {
    if (sTank <= threshold) {
        sTank = threshold;
        sRecharging = true;
        return true;
    }
    return false;
}

float FluddTwist::stopTankValue() {
    return (sTank >= sTankRunoutVal) ? sTank - sTankRunoutVal : 0.f;
}

float FluddTwist::smoothVelocity(float from, float to, float g) {
    if (g < 0) {
        if ((to + 2) < from)
            return from;
        if ((to + 2) > from)
            return al::lerpValue(from, to, 0.8f);
        return to;
    } else if (g > 0) {
        if ((to + 2) > from)
            return from;
        if ((to + 2) < from)
            return al::lerpValue(from, to, 0.8f);
        return to;
    }
    return from;
}

void FluddTwist::changeFluddModeL() {
    sFluddMode++;
    setFluddModeValues();
}

void FluddTwist::changeFluddModeR() {
    sFluddMode--;
    setFluddModeValues();
}

void FluddTwist::setFluddModeValues() {
    if (sFluddMode == 0 || sFluddMode > 2) {
        sFluddMode = 0;
        sFluddVel = 2.8f;
        sFluddRecharge = 0.4f;
        sFluddDischarge = 0.1f;
        sChargeTimer = 0.f;
        sTankRunoutVal = 34.f;
        sTStopValueSet = false;
        sRecharging = false;
        if (!sIsHack && !sIs2D && !rs::isActiveDemo(sMario)) {
            al::showModel(sHover);
            al::hideModel(sRocket);
            al::hideModel(sTurbo);
        }
    } else if (sFluddMode == 1) {
        sFluddVel = 90.f;
        sFluddRecharge = 0.4f;
        sChargeTimer = 100.f;
        sChargeTimerDecrease = 1.5f;
        sTankRunoutVal = 33.3f;
        sFluddDischarge = sTankRunoutVal / 2.f;
        sTStopValueSet = false;
        sRecharging = false;
        sIsFirstBoost = true;
        if (!sIsHack && !sIs2D && !rs::isActiveDemo(sMario)) {
            al::showModel(sRocket);
            al::hideModel(sHover);
            al::hideModel(sTurbo);
        }
    } else if (sFluddMode == 2 || sFluddMode < 0) {
        sFluddMode = 2;
        sFluddDischarge = 0.12f;
        sFluddRecharge = 0.4f;
        sChargeTimer = 100.f;
        sChargeTimerDecrease = 1.f;
        sTankRunoutVal = 100.f;
        sTStopValueSet = false;
        sRecharging = false;
        if (!sIsHack && !sIs2D && !rs::isActiveDemo(sMario)) {
            al::showModel(sTurbo);
            al::hideModel(sRocket);
            al::hideModel(sHover);
        }
    }
}

bool FluddTwist::canFluddActivate() {
    if (sIsHack || PlayerFunction::isPlayerDeadStatus(sMario) || rs::isActiveDemo(sMario) || isTankEmpty(sTankStopValue))
        return false;

    if (sChargeTimer > 0.f)
        sChargeTimer -= sChargeTimerDecrease;
    else
        sChargeTimer = 0.f;

    if (sFluddMode == 0) {
        sRocket->deactivate();
        sTurbo->deactivate();
        if (!sIsPGrounded && !sIsPUnderWater)
            return true;
        if (sIsPGrounded)
            sHover->deactivate();
    }
    if (sFluddMode == 1) {
        sTurbo->deactivate();
        sHover->deactivate();
        if (!sIsPInWater && !sIsPGrounded)
            return true;
    }
    if (sFluddMode == 2) {
        sRocket->deactivate();
        sHover->deactivate();
        if (sMario->mInput->isMove())
            return true;
    }
    return false;
}

void FluddTwist::activateFludd() {
    if (sFluddMode == 0) {
        if (!sIs2D)
            al::setVelocityY(sMario, smoothVelocity(al::getVelocity(sMario).y, sFluddVel, -1));
        else {
            sead::Vector3f g = al::getGravity(sMario);
            al::setVelocityY(sMario, smoothVelocity(al::getVelocity(sMario).y, -1 * g.y * sFluddVel, g.y));
        }
        if (sMario->mAnimator->isAnim("JumpBroad") || sMario->mAnimator->isAnim("JumpBroad2") || sMario->mAnimator->isAnim("JumpBroad3") ||
            sMario->mAnimator->isAnim("HeadSliding") || sMario->mAnimator->isAnim("DiveInWater") || sMario->mAnimator->isAnim("HeadSlidingStart"))
            sMario->mAnimator->startAnim(kHoverAnim);
        sBase->activate();
        sHover->activate(true);
        sTank -= sFluddDischarge;

    } else if (sFluddMode == 1 && !sIsPGrounded) {
        if (al::isPadHoldA(-1)) {
            if (sIsFirstBoost) {
                al::setVelocityY(sMario, sFluddVel);
                if (sMario->mAnimator->isAnim("JumpBroad") || sMario->mAnimator->isAnim("JumpBroad2") || sMario->mAnimator->isAnim("JumpBroad3") ||
                    sMario->mAnimator->isAnim("HeadSliding") || sMario->mAnimator->isAnim("DiveInWater") || sMario->mAnimator->isAnim("HeadSlidingStart"))
                    sMario->mAnimator->startAnim(kHoverAnim);
                if (!sIs2D) {
                    sBase->activate();
                    sRocket->activate(true);
                }
                sLJCancel = true;
                sTank -= sFluddDischarge;
                sIsFirstBoost = false;
            } else {
                sDoubleBoostFrames++;
                if (sDoubleBoostFrames >= 2)
                    sTank -= sFluddDischarge;
            }
        } else {
            if (sDoubleBoostFrames < 2 && !sIsFirstBoost) {
                sIsFirstBoost = true;
                sDoubleBoostFrames = 0;
            }
        }

    } else if (sFluddMode == 2) {
        if (sIsPInWater) {
            al::setVelocityToFront(sMario, 30.f);
            if (al::isPadHoldA(-1))
                al::setVelocityY(sMario, 8.f);
            else if (al::isPadHoldB(-1))
                al::setVelocityY(sMario, -8.f);
            sBase->activate();
            if (sIsPGrounded)
                sTurbo->activate(true);
            else {
                sTurbo->activate(false);
                al::tryEmitEffect(sMario->mModelHolder->findModelActor("Normal"), "StateIce", al::getTransPtr(sMario));
            }
            sTank -= sFluddDischarge / 2.f;
        } else if (sIsPGrounded) {
            if (sSetNrvGrounded) {
                sMario->setNerveOnGround();
                sSetNrvGrounded = false;
            }
            al::setVelocityToFront(sMario, 45.f);
            al::setVelocityY(sMario, -5.f);
            sBase->activate();
            if (!sIs2D)
                sTurbo->activate(true);
            sTank -= sFluddDischarge;
        }
    }
}

void FluddTwist::deactivateFludd() {
    sBase->deactivate();
    sHover->deactivate();
    if (al::getVelocity(sMario).y < 5.f)
        sRocket->deactivate();
    sTurbo->deactivate();
    al::tryDeleteEffect(sMario->mModelHolder->findModelActor("Normal"), "StateIce");

    if (sChargeTimer < 100.f && sFluddMode != 0)
        sChargeTimer += 0.6f;

    if (sIsPGrounded || sIsPInWater) {
        if (sIsPInWater)
            fluddTankFill();

        if (sTank < 100.f && sFluddMode == 0)
            sTank += sFluddRecharge;
        else if (sTank < 100.f && sFluddMode != 0 && sChargeTimer >= 100.f) {
            sChargeTimer = 100.f;
            sTank += sFluddRecharge;
        }

        if (sFluddMode == 0) {
            sRecharging = false;
        } else if (sFluddMode != 0 && sChargeTimer >= 100.f) {
            sChargeTimer = 100.f;
            sIsFirstBoost = true;
            sDoubleBoostFrames = 0;
            sRecharging = false;
        }

        if (!sRecharging && sTStopValueSet)
            sTStopValueSet = false;
    }
}