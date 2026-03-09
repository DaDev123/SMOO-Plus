/**
 * @file TwoD.hpp
 * @brief Forces Mario into 2D mode, with R+Up triggering a timed 3D swap.
 *        Ported from 2Dyssey DimensionPatcher. - Amethyst-SZS
 */

#pragma once

#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Player/PlayerAnimator.h"
#include "game/Scene/StageScene.h"
#include "game/Util/ActorDimensionKeeper.h"

class PlayerFormSensorCollisionArranger {
public:
    void setFormModel2D();
    void setFormModel3D();
};

class TwoDTwist {
public:
    static bool sTwoDEnabled;
    static bool isTwoDEnabled() { return sTwoDEnabled; }
    static void toggleTwoD();

    static void initHooks();  // call once at boot from hkMain
    static void onStageInit(StageScene* scene, PlayerActorHakoniwa* p1);
    static void onStageDeath();
    static void update(StageScene* scene, PlayerActorHakoniwa* p1, bool isFirstStep);

    static ActorDimensionKeeper* sDimensionKeeper;
    static PlayerActorHakoniwa* sPlayer;
    static int sSceneFrames;

    static bool isIn2DMode();
};