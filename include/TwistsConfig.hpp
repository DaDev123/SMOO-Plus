#pragma once

// Forward declarations
class PlayerActorHakoniwa;
class StageScene;

class TwistsConfig {
public:
    // Twist toggle states
    static bool sCappyForceEnabled;

    // Getters
    static bool isCappyDisableEnabled();

    // Setters
    static void toggleCappyDisable();

    // Update function for Cappy proximity logic
    static void updateCappyProximity(PlayerActorHakoniwa* player, StageScene* stageScene);
    static void handleStageInit();

private:
    static bool cappyDisabled;
    static bool needsCappyDisable;
    static float cappyThreshold;
};