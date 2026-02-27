#include "TwistsConfig.hpp"

#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/LiveActor.h"

#include "game/MapObj/ShineTowerRocket.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Scene/StageScene.h"
#include "game/Sequence/ChangeStageInfo.h"
#include "game/System/GameDataFile.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolderAccessor.h"
#include "game/System/GameDataHolderWriter.h"
#include "game/Util/DemoUtil.h"

#include "../src/smallMarioHooks.hpp"
#include "logger.hpp"
#include "server/Client.hpp"

// Initialize static variables
bool TwistsConfig::sCappyForceEnabled = true;  // Changed to true so Cappy is enabled by default
bool TwistsConfig::cappyDisabled = false;      // Changed to false since Cappy starts enabled
bool TwistsConfig::needsCappyDisable = false;  // Changed to false since we don't need to disable
float TwistsConfig::cappyThreshold = 500.0f;   // Adjust as needed

bool TwistsConfig::sIcePhysicsEnabled = false;
bool TwistsConfig::sSmallMarioEnabled = false;

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
    if (!player || !stageScene)
        return;

    PlayerActorBase* playerBase = (PlayerActorBase*)rs::getPlayerActor(stageScene);
    if (!playerBase)
        return;

    GameDataHolderWriter writer(stageScene);

    GameDataHolderAccessor accessor(stageScene);
    bool isCappyCurrentlyEnabled = GameDataFunction::isEnableCap(accessor);

    ShineTowerRocket* odyssey = rs::tryGetShineTowerRocketFromDemoDirector((al::LiveActor*)playerBase);

    if (!sCappyForceEnabled) {
        // Toggle is OFF - use normal proximity logic
        if (!odyssey) {
            if (isCappyCurrentlyEnabled) {
                writer.mData->getGameDataFile()->getIsEnableCap() = false;
                cappyDisabled = true;
                weDisabledCappy = true;
                Logger::log("Cappy disabled - no Odyssey\n");
            }
            return;
        }

        f32 distance = al::calcDistance((al::LiveActor*)player, (al::LiveActor*)odyssey);
        bool shouldEnable = (distance <= cappyThreshold);

        if (shouldEnable && !isCappyCurrentlyEnabled) {
            writer.mData->getGameDataFile()->getIsEnableCap() = true;
            cappyDisabled = false;
            weDisabledCappy = false;
            Logger::log("Cappy enabled - near Odyssey (%.1f)\n", distance);
        } else if (!shouldEnable && isCappyCurrentlyEnabled) {
            writer.mData->getGameDataFile()->getIsEnableCap() = false;
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
            writer.mData->getGameDataFile()->getIsEnableCap() = true;
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
    cappyDisabled = false;                               // Changed: Cappy starts enabled
    needsCappyDisable = false;                           // Changed: We don't need to disable it
    Logger::log("Stage init: Cappy will be enabled\n");  // Updated log message
}

void TwistsConfig::toggleSmallMario() {
    sSmallMarioEnabled = !sSmallMarioEnabled;
    if (sSmallMarioEnabled)
        smallMario::installHooks();
    else
        smallMario::uninstallHooks();

    ChangeStageInfo info =
        ChangeStageInfo(Client::get()->getHolder(), Client::get()->getHolder()->getGameDataFile()->getPlayerStartId().cstr(),
                        GameDataFunction::getCurrentStageName(Client::get()->getHolder()), false, -1, ChangeStageInfo::SubScenarioType::NO_SUB_SCENARIO);
    Client::get()->getHolder()->changeNextStage(&info, 0);
}