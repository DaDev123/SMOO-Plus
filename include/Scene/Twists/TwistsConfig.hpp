#pragma once

#include "Scene/Twists/Cappyless/Cappyless.hpp"
#include "Scene/Twists/Darkness/Darkness.hpp"
#include "Scene/Twists/IcePhysics/IcePhysics.hpp"
#include "Scene/Twists/SmallMario/SmallMario.hpp"
#include "Scene/Twists/Timewarp/Timewarp.hpp"

class PlayerActorHakoniwa;
class StageScene;

namespace al {
class Triangle;
}

class TwistsConfig {
public:
    // --- Cappy ---
    static bool isCappyDisableEnabled() { return Cappyless::isCappyDisableEnabled(); }
    static void toggleCappyDisable() { Cappyless::toggleCappyDisable(); }
    static void updateCappyProximity(PlayerActorHakoniwa* player, StageScene* stageScene) { Cappyless::updateCappyProximity(player, stageScene); }
    static void handleStageInit() { Cappyless::handleStageInit(); }

    // --- Ice Physics ---
    static bool isIcePhysicsEnabled() { return IcePhysics::isIcePhysicsEnabled(); }
    static void toggleIcePhysics() { IcePhysics::toggleIcePhysics(); }
    static bool shouldUseIcePhysics(bool originalFloorCheck) { return IcePhysics::shouldUseIcePhysics(originalFloorCheck); }

    // --- Small Mario ---
    static bool isSmallMarioEnabled() { return SmallMario::isSmallMarioEnabled(); }
    static void toggleSmallMario() { SmallMario::toggleSmallMario(); }

    // --- Darkness ---
    static bool isDarknessEnabled() { return DarknessTwist::isDarknessEnabled(); }
    static void toggleDarkness() { DarknessTwist::toggleDarkness(); }

    // --- Time Warp ---
    static bool isTimeWarpEnabled() { return TimeWarpTwist::isTimeWarpEnabled(); }
    static void toggleTimeWarp() { TimeWarpTwist::toggleTimeWarp(); }
};