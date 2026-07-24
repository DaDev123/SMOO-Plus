#include "hk/hook/Trampoline.h"

#include "sead/prim/seadSafeString.h"

#include "al/Library/Memory/HeapUtil.h"
#include "al/Library/Nerve/NerveUtil.h"

#include "game/Layout/FooterParts.h"
#include "game/Scene/StageSceneStatePauseMenu.h"

#include "Scene/StageSceneStateModConfig.hpp"
#include "server/Client.hpp"

static bool isModMenu = false;

HkReplace<void, StageSceneStatePauseMenu*> overrideHelpFadeNerve =
    hk::hook::replace([](StageSceneStatePauseMenu* state) -> void {
        isModMenu = true;
        // Set label in menu inside LocalizedData/${lang}/MessageData/LayoutMessage.szs/Menu.msbt/Menu_Help
        al::setNerve(state, &NrvStageSceneStatePauseMenu.ModConfig);
    });

HkTrampoline initNerveStateHook =
    [](TrampolineStatic(), StageSceneStatePauseMenu* state, const char* name, al::Scene* host,
       al::SimpleLayoutAppearWaitEnd* menuLayout, GameDataHolder* gameDataHolder,
       const al::SceneInitInfo& sceneInitInfo, const al::ActorInitInfo& actorInitInfo,
       const al::LayoutInitInfo& layoutInitInfo, al::WindowConfirm* windowConfirm,
       StageSceneLayout* stageSceneLayout, bool isTitle,
       SceneAudioSystemPauseController* sceneAudioSystemPauseController) -> void {
    orig(state, name, host, menuLayout, gameDataHolder, sceneInitInfo, actorInitInfo, layoutInitInfo,
         windowConfirm, stageSceneLayout, isTitle, sceneAudioSystemPauseController);

    StageSceneStateModConfig* sceneStateModConfig = new (al::getSceneHeap())
        StageSceneStateModConfig("ModConfig", host, layoutInitInfo, state->mFooterParts, gameDataHolder);

    al::initNerveState(state, sceneStateModConfig, &NrvStageSceneStatePauseMenu.ModConfig,
                       "CustomNerveOverride");
};

HkTrampoline pauseMenuAppearHook = [](TrampolineStatic(), StageSceneStatePauseMenu* menu) -> void {
    if (al::isFirstStep(menu))
        menu->mSelectParts->setSelectMessage(2, u"Mod Menu");

    orig(menu);
};

HkTrampoline pauseMenuWaitHook = [](TrampolineStatic(), StageSceneStatePauseMenu* menu) -> void {
    orig(menu);

    if (!al::isNerve(menu, &NrvStageSceneStatePauseMenu.ModConfig)) {
        isModMenu = false;
    }
    static char16_t buf[0x200];
    static sead::WBufferedSafeString baseStr{buf, 0x100};
    static sead::WBufferedSafeString newStr{buf + 0x100, 0x100};
    if (al::isFirstStep(menu)) {
        if (baseStr.isEmpty())
            baseStr = menu->mFooterParts->mText;

        menu->mSelectParts->setSelectMessage(2, u"Mod Menu");
        bool online = Client::isSocketActive();

        online ? newStr.format(u"Online: %d/%d", Client::getConnectCount() + 1, Client::getMaxPlayerCount()) :
                 newStr.format(u"Offline");
        newStr.append(u"\t\t\t\t\t\t\t\t");
        newStr.append(baseStr.cstr());

        menu->mFooterParts->changeText(newStr.cstr());
    }
};

HkTrampoline shadowHook = [](TrampolineStatic(), void* a) -> void {
    if (isModMenu)
        return;
    orig(a);
};

void installModMenuHooks() {
    // increase nerve state count to 5
    hk::hook::a64::assemble<"mov w2, #5">().installAtSym<"R_ZN24StageSceneStatePauseMenuNrvStateCount">();

    // inits options nerve state and server config state
    initNerveStateHook.installAtSym<"R_ZN24StageSceneStatePauseMenuC1">();

    // Change Action Guide Text
    pauseMenuAppearHook.installAtSym<"_ZN24StageSceneStatePauseMenu9exeAppearEv">();

    // Onine Indicator
    pauseMenuWaitHook.installAtSym<"_ZN24StageSceneStatePauseMenu7exeWaitEv">();

    // Override Action Guide Button
    overrideHelpFadeNerve.installAtSym<"_ZN24StageSceneStatePauseMenu17exeFadeBeforeHelpEv">();

    // fix stupid crash
    shadowHook.installAtSym<"_ZN2al14ShadowDirector6updateEv">();
}