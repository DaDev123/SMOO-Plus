#include "layouts/FluddIcon.hpp"

#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/Layout/LayoutActorUtil.h"
#include "al/Library/Nerve/NerveUtil.h"

FluddIcon* FluddIcon::sInstance = nullptr;

// Pane names expected in the FluddIcon.szs layout:
//   TxtMode     - text pane showing current nozzle name
//   TxtTank     - text pane showing tank % (optional debug)
//   GaugeBar    - pane scaled on X axis to show tank level (0.0–1.0)
//   IcoHover    - icon pane shown when mode == 0 (Hover)
//   IcoRocket   - icon pane shown when mode == 1 (Rocket)
//   IcoTurbo    - icon pane shown when mode == 2 (Turbo)
//   DotRecharge - indicator shown while tank is recharging

FluddIcon::FluddIcon(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {
    al::initLayoutActor(this, initInfo, "FluddIcon", 0);

    initNerve(&NrvFluddIcon.End, 0);

    kill();
}

void FluddIcon::appear() {
    al::startAction(this, "Appear", 0);
    al::setNerve(this, &NrvFluddIcon.Appear);
    al::LayoutActor::appear();
}

bool FluddIcon::tryStart() {
    if (!al::isNerve(this, &NrvFluddIcon.Wait) && !al::isNerve(this, &NrvFluddIcon.Appear)) {
        appear();
        return true;
    }
    return false;
}

bool FluddIcon::tryEnd() {
    if (!al::isNerve(this, &NrvFluddIcon.End)) {
        al::setNerve(this, &NrvFluddIcon.End);
        return true;
    }
    return false;
}

void FluddIcon::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &NrvFluddIcon.Wait);
    }
}

void FluddIcon::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }
    updateDisplay();
}

void FluddIcon::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void FluddIcon::updateDisplay() {
    int mode = FluddTwist::getFluddMode();
    float tank = FluddTwist::getTank();
    bool recharge = FluddTwist::isRecharging();
    bool stickActive = FluddTwist::isStickActive();

    // --- LStick toggle indicator ---
    if (al::isExistPane(this, "LStick"))
        stickActive ? al::showPane(this, "LStick") : al::hidePane(this, "LStick");

    // --- Nozzle icons: hide all then show active ---
    if (al::isExistPane(this, "FLUDDHover"))
        mode == 0 ? al::showPane(this, "FLUDDHover") : al::hidePane(this, "FLUDDHover");
    if (al::isExistPane(this, "FLUDDRocket"))
        mode == 1 ? al::showPane(this, "FLUDDRocket") : al::hidePane(this, "FLUDDRocket");
    if (al::isExistPane(this, "FLUDDTurbo"))
        mode == 2 ? al::showPane(this, "FLUDDTurbo") : al::hidePane(this, "FLUDDTurbo");

    // --- ChargeBar: only shown for Rocket/Turbo while charging ---
    if (al::isExistPane(this, "ChargeBar")) {
        float chargeTimer = FluddTwist::getChargeTimer();
        if (mode != 0 && chargeTimer < 100.f)
            al::showPane(this, "ChargeBar");
        else
            al::hidePane(this, "ChargeBar");
    }

    // --- Tank water level ---
    float tankVal = tank / 100.f;
    if (al::isExistPane(this, "TankWater")) {
        float tankTrans = -39.f - (118.f * (1.f - tankVal));
        al::setPaneLocalTrans(this, "TankWater", sead::Vector2f(3.5f, tankTrans));
        al::setPaneLocalScale(this, "TankWater", sead::Vector2f(1.f, tankVal));
    }

    // --- Tank stop bar ---
    if (al::isExistPane(this, "TankWaterStop")) {
        float tankStopVal = FluddTwist::getTankStopValue() / 100.f;
        float tankStopLevel = -160.f + (237.f * tankStopVal);
        al::setPaneLocalTrans(this, "TankWaterStop", sead::Vector2f(-10.4f, tankStopLevel));
    }

    // --- ChargeBar trans/scale ---
    if (al::isExistPane(this, "ChargeBar")) {
        float chargeTimer = FluddTwist::getChargeTimer();
        float cT = chargeTimer / 100.f;
        float cBarTrans = -43.f - (119.f * (1.f - cT));
        al::setPaneLocalTrans(this, "ChargeBar", sead::Vector2f(0.f, cBarTrans));
        al::setPaneLocalScale(this, "ChargeBar", sead::Vector2f(1.f, cT));
    }
}