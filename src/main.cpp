#include <al/Library/Memory/HeapUtil.h>
#include <cstddef>
#include <game/Sequence/HakoniwaSequence.h>
#include <game/System/Application.h>
#include <game/System/GameFrameworkNx.h>
#include <game/System/GameSystem.h>
#include <hk/hook/Trampoline.h>
#include <nn/fs.h>
#include <nvnImGui/imgui_nvn.h>
#include <sead/filedevice/nin/seadNinSDFileDeviceNin.h>
#include <sead/filedevice/seadFileDeviceMgr.h>
#include <sead/heap/seadExpHeap.h>

#include "helpers.h"
#include "Menu.h"
#include "saveFileHelper.h"
#include "settings/SettingsHooks.h"
#include "settings/SettingsMgr.h"

#include "imgui.h"

using namespace hk;
using namespace btt;

static sead::Heap* sBTTStudioHeap = nullptr;

void drawFpsWindow() {
    if (!Menu::instance()->mIsEnabledMenu) return;
    ImGui::SetNextWindowPos(ImVec2(120.f, -8.f));

    ImGui::Begin(
        "FPSCounter", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoBackground
    );

    ImGui::Text("FPS: %2.f\n", Application::instance()->mGameFramework->calcFps());

    ImGui::End();
}

void drawMenu() {
    Menu* menu = Menu::instance();
    if (menu && menu->mIsEnabledMenu) menu->draw();
    menu->handleAlways();
}

HkTrampoline<void, GameSystem*> gameSystemInit = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
    sBTTStudioHeap = sead::ExpHeap::create(1_MB, "BTTStudioHeap", al::getStationedHeap(), 8, sead::Heap::cHeapDirection_Forward, false);

    SettingsMgr* set = SettingsMgr::createInstance(sBTTStudioHeap);
    SaveFileHelper::createInstance(sBTTStudioHeap);
    Menu* menu = Menu::createInstance(sBTTStudioHeap);
    

    menu->setupStyle();

    nvnImGui::addDrawFunc(drawMenu);
    // nvnImGui::addDrawFunc(drawFpsWindow);

    InputHelper::setDisableMouse(true);

    gameSystemInit.orig(gameSystem);

    SaveFileHelper::instance()->loadSettings();
    SaveFileHelper::instance()->loadTeleport(menu->tpStates, hk::util::arraySize(menu->tpStates));
});

HkTrampoline<void, GameSystem*> drawMainHook = hk::hook::trampoline([](GameSystem* gameSystem) -> void { drawMainHook.orig(gameSystem); });

HkTrampoline<void, sead::FileDeviceMgr*> fileDeviceMgrHook = hk::hook::trampoline([](sead::FileDeviceMgr* fileDeviceMgr) -> void {
    fileDeviceMgrHook.orig(fileDeviceMgr);

    fileDeviceMgr->mMountedSd = nn::fs::MountSdCardForDebug("sd") == 0;
});
int timer = 0;
HkTrampoline<void, HakoniwaSequence*> hakoniwaSequenceUpdate = hk::hook::trampoline([](HakoniwaSequence* hakoniwaSequence) -> void {
    hakoniwaSequenceUpdate.orig(hakoniwaSequence);

    Menu* menu = Menu::instance();
    
    if (menu->globalTimer % 3600 == 0) {
        SaveFileHelper::instance()->saveSettings();
        // SaveFileHelper::instance()->saveTeleport(Menu::instance()->tpStates, hk::util::arraySize(Menu::instance()->tpStates));
    }
    menu->globalTimer++;
});

extern "C" void hkMain() {
    gameSystemInit.installAtSym<"_ZN10GameSystem4initEv">();
    drawMainHook.installAtSym<"_ZN10GameSystem8drawMainEv">();
    fileDeviceMgrHook.installAtSym<"_ZN4sead13FileDeviceMgrC1Ev">();
    hakoniwaSequenceUpdate.installAtSym<"_ZN16HakoniwaSequence6updateEv">();

    SettingsHooks::installSettingsHooks();

    nvnImGui::InstallHooks();
}
