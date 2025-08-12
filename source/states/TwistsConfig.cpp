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

static bool weDisabledCappy = false;

void TwistsConfig::updateCappyProximity(PlayerActorHakoniwa* player, StageScene* stageScene) {
    if (!player || !stageScene) return;

    PlayerActorBase* playerBase = rs::getPlayerActor(stageScene);
    if (!playerBase) return;

    GameDataHolderWriter writer;
    writer.mData = stageScene->mHolder.mData;
    
    GameDataHolderAccessor accessor(stageScene);
    bool isCappyCurrentlyEnabled = GameDataFunction::isEnableCap(accessor);
    
    ShineTowerRocket* odyssey = rs::tryGetShineTowerRocketFromDemoDirector((al::LiveActor*)playerBase);

    if (!sCappyForceEnabled) {
        // Toggle is OFF - use normal proximity logic
        if (!odyssey) {
            if (isCappyCurrentlyEnabled) {
                writer.mData->mGameDataFile->mIsEnableCap = false;
                cappyDisabled = true;
                weDisabledCappy = true;
                Logger::log("Cappy disabled - no Odyssey\n");
            }
            return;
        }

        f32 distance = al::calcDistance((al::LiveActor*)player, (al::LiveActor*)odyssey);
        bool shouldEnable = (distance <= cappyThreshold);

        if (shouldEnable && !isCappyCurrentlyEnabled) {
            writer.mData->mGameDataFile->mIsEnableCap = true;
            cappyDisabled = false;
            weDisabledCappy = false;
            Logger::log("Cappy enabled - near Odyssey (%.1f)\n", distance);
        } else if (!shouldEnable && isCappyCurrentlyEnabled) {
            writer.mData->mGameDataFile->mIsEnableCap = false;
            cappyDisabled = true;
            weDisabledCappy = true;
            Logger::log("Cappy disabled - far from Odyssey (%.1f)\n", distance);
        }
        return;
    }

    // Toggle is ON - force enable, but only if we're not in a naturally capless area
    
    if (!isCappyCurrentlyEnabled) {
        // Cappy is disabled. Was it us who disabled it, or the game naturally?
        if (weDisabledCappy || cappyDisabled) {
            // We disabled it due to proximity, safe to re-enable
            writer.mData->mGameDataFile->mIsEnableCap = true;
            cappyDisabled = false;
            weDisabledCappy = false;
            Logger::log("Cappy forced enabled by toggle\n");
        } else {
            // Game naturally disabled it (capless area), don't override
            Logger::log("Cappy remains disabled - naturally disabled by game\n");
        }
    }
    // If Cappy is already enabled, no need to do anything
}

void TwistsConfig::handleStageInit() {
    cappyDisabled = false;      // Changed: Cappy starts enabled
    needsCappyDisable = false;  // Changed: We don't need to disable it
    Logger::log("Stage init: Cappy will be enabled\n");  // Updated log message
}
