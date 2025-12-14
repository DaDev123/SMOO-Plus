#pragma once

// Forward declarations
class PlayerActorHakoniwa;
class StageScene;

namespace al {
    class Triangle;
}

class TwistsConfig {
public:
    // Twist toggle states
    static bool sCappyForceEnabled;
    static bool sIcePhysicsEnabled;   // Declare here (no extern!)

    // Getters
    static bool isCappyDisableEnabled();
    static bool isIcePhysicsEnabled() { return sIcePhysicsEnabled; }
    static bool shouldUseIcePhysics(bool originalFloorCheck);

    // Setters
    static void toggleCappyDisable();
    static void toggleIcePhysics() { sIcePhysicsEnabled = !sIcePhysicsEnabled; }

    // Update functions
    static void updateCappyProximity(PlayerActorHakoniwa* player, StageScene* stageScene);
    static void handleStageInit();

private:
    static bool cappyDisabled;
    static bool needsCappyDisable;
    static float cappyThreshold;
};

// Your patch function that combines original check with toggle
// Changed from pointer (*) to reference (&)
bool icePhysicsPatch(const al::Triangle& triangle, const char* floorCode);