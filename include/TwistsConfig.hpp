#pragma once

class PlayerActorHakoniwa;
class StageScene;

namespace al {
class Triangle;
}

class TwistsConfig {
public:
    static bool sCappyForceEnabled;
    static bool sIcePhysicsEnabled;
    static bool sSmallMarioEnabled;

    static bool isCappyDisableEnabled();
    static bool isIcePhysicsEnabled() { return sIcePhysicsEnabled; }
    static bool isSmallMarioEnabled() { return sSmallMarioEnabled; }
    static bool shouldUseIcePhysics(bool originalFloorCheck);

    static void toggleCappyDisable();
    static void toggleIcePhysics() { sIcePhysicsEnabled = !sIcePhysicsEnabled; }
    static void toggleSmallMario();

    static void updateCappyProximity(PlayerActorHakoniwa* player, StageScene* stageScene);
    static void handleStageInit();

private:
    static bool cappyDisabled;
    static bool needsCappyDisable;
    static float cappyThreshold;
};

bool icePhysicsPatch(const al::Triangle& triangle, const char* floorCode);