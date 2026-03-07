#pragma once

class PlayerActorHakoniwa;
class StageScene;

class Cappyless {
public:
    static bool sCappyForceEnabled;

    static bool isCappyDisableEnabled();
    static void toggleCappyDisable();

    static void updateCappyProximity(PlayerActorHakoniwa* player, StageScene* stageScene);
    static void handleStageInit();

private:
    static bool cappyDisabled;
    static bool needsCappyDisable;
    static float cappyThreshold;
};