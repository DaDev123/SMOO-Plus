#include "Scene/Twists/Darkness/Darkness.hpp"

#include "hk/hook/Trampoline.h"

#include "al/Library/Base/StringUtil.h"
#include "al/Library/Scene/Scene.h"

#include "game/MapObj/ChangeStageInfo.h"
#include "game/System/GameDataFunction.h"

#include "server/Client.hpp"

bool DarknessTwist::sDarknessEnabled = false;

static constexpr const char* OVERRIDE_GRAPHICS_STAGE_NAME = "DonsukeExStage";

static HkTrampoline<void, al::Scene*, const char*, int> initGraphicsSystemInfoHook =
    hk::hook::trampoline([](al::Scene* scene, const char* stageName, int maybeScenario) -> void {
        if (DarknessTwist::sDarknessEnabled)
            stageName = OVERRIDE_GRAPHICS_STAGE_NAME;
        initGraphicsSystemInfoHook.orig(scene, stageName, maybeScenario);
    });

static HkTrampoline<void, void*, const char*, int, const char*> stageResourceListCtorHook =
    hk::hook::trampoline([](void* thisPtr, const char* stageName, int maybeScenario, const char* levelFile) -> void {
        if (DarknessTwist::sDarknessEnabled && al::isEqualString(levelFile, "Design"))
            stageName = OVERRIDE_GRAPHICS_STAGE_NAME;
        stageResourceListCtorHook.orig(thisPtr, stageName, maybeScenario, levelFile);
    });

void DarknessTwist::toggleDarkness() {
    sDarknessEnabled = !sDarknessEnabled;

    ChangeStageInfo info =
        ChangeStageInfo(Client::get()->getHolder(), Client::get()->getHolder()->getGameDataFile()->getPlayerStartId().cstr(),
                        GameDataFunction::getCurrentStageName(Client::get()->getHolder()), false, -1, ChangeStageInfo::SubScenarioType::NO_SUB_SCENARIO);
    Client::get()->getHolder()->changeNextStage(&info, 0);
}

void DarknessTwist::initHooks() {
    initGraphicsSystemInfoHook.installAtSym<"_ZN2al22initGraphicsSystemInfoEPNS_5SceneEPKci">();
    stageResourceListCtorHook.installAtSym<"_ZN2al17StageResourceListC2EPKciS2_">();
}
