#include "hk/hook/Trampoline.h"

#include "al/Library/Base/StringUtil.h"
#include "al/Library/Stage/IUseStageSwitch.h"
#include "al/Library/Thread/FunctorV0M.h"

#include "game/MapObj/AppearSwitchTimer.h"

#include "Scene/StageSceneStateModConfig.hpp"

HkTrampoline unlockCostumeDoorsHook = [](TrampolineStatic(), al::IUseStageSwitch* user, const char* eventName,
                                         const al::FunctorBase& action) -> bool {
    if (strcmp(eventName, "OpenKeySwitch") == 0 && StageSceneStateModConfig::isCostumeDoorsUnlocked())
        return false;
    return orig(user, eventName, action);
};

bool unlockCostumeDoorMetroHook(const char* str1, const char* str2) {
    if (StageSceneStateModConfig::isCostumeDoorsUnlocked())
        return true;
    return al::isEqualString(str1, str2);
}

HkTrampoline disableAppearSwitchCameraHook =
    [](TrampolineStatic(), AppearSwitchTimer* timer, const al::ActorInitInfo& initInofo,
       const al::IUseAudioKeeper* audio, al::IUseStageSwitch* stageSwitch, al::IUseCamera* camera,
       al::LiveActor* actor) -> void {
    orig(timer, initInofo, audio, stageSwitch, camera, actor);
    timer->mDemoCameraFrame = 0;
};

void installQolHooks() {
    // Amiibo Button Disabling
    hk::hook::replace([] {}).installAtSym<"_ZN2rs16isHoldAmiiboModeEPKN2al18IUseSceneObjHolderE">();
    hk::hook::replace([] {}).installAtSym<"_ZN2rs19isTriggerAmiiboModeEPKN2al18IUseSceneObjHolderE">();

    // unlock costume doors
    unlockCostumeDoorsHook.installAtSym<
        "_ZN2al19listenStageSwitchOnEPNS_15IUseStageSwitchEPKcRKNS_11FunctorBaseE">();  // all except metro
    hk::hook::writeBranchLinkAtSym<"R_metroCostumeDoor">(unlockCostumeDoorMetroHook);   // metro

    // Disable Action Guide / HtmlViewer
    hk::hook::trampoline([] {}).installAtSym<"_ZN2rs21requestShowHtmlViewerEPKN2al18IUseSceneObjHolderE">();

    // disables AppearSwitchTimer's camera switch
    disableAppearSwitchCameraHook.installAtSym<"R_ZN17AppearSwitchTimer4init">();

    // always allow checkpoint warps
    hk::hook::trampoline([] { return true; }).installAtSym<"_ZNK9MapLayout22isEnableCheckpointWarpEv">();

    // hk::hook::a64::assemble<"nop">().installAtMainOffset(0x45c69c);  // Removes Assist Mode Ledge Grabs
}