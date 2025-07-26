#include "TwistsConfig.hpp"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/StageScene/StageScene.h"
#include "game/GameData/GameDataHolderWriter.h"
#include "game/GameData/GameDataHolderAccessor.h"
#include "game/GameData/GameDataFunction.h"
#include "al/LiveActor/LiveActor.h"
#include "al/util/MathUtil.h"
#include "al/util/DemoUtil.h"
#include "al/util.hpp"
#include "rs/util.hpp"
#include "al/util/LiveActorUtil.h"
#include "al/util/StringUtil.h"
#include "logger.hpp"


// Initialize static variables
bool TwistsConfig::sCappyForceEnabled = true;  // Changed to true so Cappy is enabled by default
bool TwistsConfig::cappyDisabled = false;      // Changed to false since Cappy starts enabled
bool TwistsConfig::needsCappyDisable = false;  // Changed to false since we don't need to disable
float TwistsConfig::cappyThreshold = 500.0f; // Adjust as needed

// Getters
bool TwistsConfig::isCappyDisableEnabled() {
    return sCappyForceEnabled;
}

// Setters
void TwistsConfig::toggleCappyDisable() {
    sCappyForceEnabled = !sCappyForceEnabled;
}

void TwistsConfig::updateCappyProximity(PlayerActorHakoniwa* player, StageScene* stageScene) {
    if (!player || !stageScene) return;

    PlayerActorBase* playerBase = rs::getPlayerActor(stageScene);
    if (!playerBase) return;

    GameDataHolderWriter writer;
    writer.mData = stageScene->mHolder.mData;

    if (sCappyForceEnabled) {
        // Forced toggle ON — force enable Cappy
        writer.mData->mGameDataFile->mIsEnableCap = true;
        cappyDisabled = false;
        Logger::log("Cappy forced enabled by toggle\n");
        return;
    } 

    // Forced toggle OFF — fallback to normal proximity logic
    GameDataHolderAccessor accessor(stageScene);
    bool isCappyEnabled = GameDataFunction::isEnableCap(accessor);
    ShineTowerRocket* odyssey = rs::tryGetShineTowerRocketFromDemoDirector((al::LiveActor*)playerBase);

    if (!odyssey) {
        if (isCappyEnabled) {
            writer.mData->mGameDataFile->mIsEnableCap = false;
            cappyDisabled = true;
            Logger::log("Cappy disabled - no Odyssey\n");
        }
        return;
    }

    f32 distance = al::calcDistance((al::LiveActor*)player, (al::LiveActor*)odyssey);
    bool shouldEnable = (distance <= cappyThreshold);

    if (shouldEnable && !isCappyEnabled) {
        writer.mData->mGameDataFile->mIsEnableCap = true;
        cappyDisabled = false;
        Logger::log("Cappy enabled - near Odyssey (%.1f)\n", distance);
    } else if (!shouldEnable && isCappyEnabled) {
        writer.mData->mGameDataFile->mIsEnableCap = false;
        cappyDisabled = true;
        Logger::log("Cappy disabled - far from Odyssey (%.1f)\n", distance);
    }
}

void TwistsConfig::handleStageInit() {
    cappyDisabled = false;      // Changed: Cappy starts enabled
    needsCappyDisable = false;  // Changed: We don't need to disable it
    Logger::log("Stage init: Cappy will be enabled\n");  // Updated log message
}