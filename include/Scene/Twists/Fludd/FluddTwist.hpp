#pragma once

#include "al/Library/LiveActor/ActorInitInfo.h"
#include "al/Library/LiveActor/LiveActor.h"

#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Scene/StageScene.h"

#include "Scene/Twists/Fludd/actors/FluddBase.hpp"
#include "Scene/Twists/Fludd/actors/FluddHover.hpp"
#include "Scene/Twists/Fludd/actors/FluddRocket.hpp"
#include "Scene/Twists/Fludd/actors/FluddTurbo.hpp"

static constexpr const char* kHoverAnim = "Fall";

class FluddTwist {
public:
    static void init(al::ActorInitInfo const& info);
    static void onStageInit(StageScene* scene);
    static void onStageDeath();
    static void toggle();
    static void update(PlayerActorHakoniwa* p1);

    static int getFluddMode();
    static float getTank();
    static float getChargeTimer();
    static float getTankStopValue();
    static bool isRecharging();
    static bool isStickActive();

    static bool sFluddEnabled;

private:
    static void setRefs();
    static void firstTimeSetup();
    static void updateModels();
    static void fluddTankFill();
    static bool isTankEmpty(float threshold);
    static float stopTankValue();
    static float smoothVelocity(float from, float to, float g);
    static void changeFluddModeL();
    static void changeFluddModeR();
    static void setFluddModeValues();
    static bool canFluddActivate();
    static void activateFludd();
    static void deactivateFludd();

    static StageScene* sStageScene;

    static FluddBase* sBase;
    static ca::FluddHover* sHover;
    static ca::FluddRocket* sRocket;
    static ca::FluddTurbo* sTurbo;

    static PlayerActorHakoniwa* sMario;
    static al::LiveActor* sMarioModel;
    static bool sIs2D;
    static bool sIsHack;
    static bool sIsPGrounded;
    static bool sIsPUnderWater;
    static bool sIsPInWater;

    static int sFluddMode;
    static float sTank;
    static float sFluddRecharge;
    static float sFluddVel;
    static float sFluddDischarge;
    static float sChargeTimer;
    static float sChargeTimerDecrease;
    static float sTankStopValue;
    static float sTankRunoutVal;
    static bool sRecharging;
    static bool sTStopValueSet;
    static bool sLJCancel;
    static bool sStickActive;
    static bool sSetNrvGrounded;
    static bool sDoOnce;
    static bool sIsFirstBoost;
    static bool sWasEverShown;  // guards hideModel calls before first showModel
    static int sDoubleBoostFrames;
};