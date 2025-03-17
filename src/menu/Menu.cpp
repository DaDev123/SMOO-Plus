#include "Menu.h"

#include "helpers/getHelper.h"
#include "InputHelper.h"

#include <cstdio>
#include <heap/seadHeapMgr.h>
#include <nn/oe.h>
#include "al/Library/Camera/CameraUtil.h"
#include "al/Library/LiveActor/ActorFlagFunction.h"
#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "game/Layout/CoinCounter.h"
#include "game/Player/PlayerHackKeeper.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerAnimator.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameSystem.h"
#include "game/Util/AchievementUtil.h"
#include "hk/types.h"
#include "hk/util/Math.h"
#include "InputDisplay.h"
#include "saveFileHelper.h"
#include "settings/SettingsMgr.h"
#include "stage_warp.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace btt {

SEAD_SINGLETON_DISPOSER_IMPL(Menu);

void Menu::draw() {
    if (InputHelper::isInputToggled()) {
        drawInputDisabled();
    }

    ImGui::Begin("SMOO+ Panel", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus);
    ImGui::SetWindowSize(mWindowSize, ImGuiCond_FirstUseEver);
    ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);

    ImGui::Text("Toggle Menu: L-Stick");
    ImGui::Text("Toggle Input: R + ZR + L");
    ImGui::Separator();

    char fmt[17] = "Toggle Mouse OFF";
    snprintf(fmt, 17, "Toggle Mouse %s", InputHelper::isDisableMouse() ? "ON" : "OFF");

    if (ImGui::Button(fmt)) {
        InputHelper::setDisableMouse(!InputHelper::isDisableMouse());
    }

    drawStageWarpWindow();

    if (ImGui::CollapsingHeader("Misc")) {
        ImGui::Indent();
        drawTeleportCat();
        drawMiscCat();
        ImGui::Unindent();
    }

    if (ImGui::CollapsingHeader("Input Display")) {
        static int bgColor = u8(set->getSettings()->mInputDisplayBackColor);
        static int ringColor = u8(set->getSettings()->mInputDisplayRingColor);
        static int stickColor = u8(set->getSettings()->mInputDisplayStickColor);
        static int buttonPressedColor = u8(set->getSettings()->mInputDisplayButtonPressedColor);
        static int buttonColor = u8(set->getSettings()->mInputDisplayButtonColor);
        static int posX = set->getSettings()->mInputDisplayPos.x;
        static int posY = set->getSettings()->mInputDisplayPos.y;

        ImGui::Indent();
        ImGui::Checkbox("Toggle", &set->getSettings()->mIsEnableInputDisplay);

        ImGui::PushItemWidth(200);

        if (ImGui::Combo("Button Color", &buttonColor, sInputDisplayColorNames, IM_ARRAYSIZE(sInputDisplayColorNames)))
            set->getSettings()->mInputDisplayButtonColor = (SettingsMgr::InputDisplayColor)buttonColor;

        if (ImGui::Combo("Pressed Color", &buttonPressedColor, sInputDisplayColorNames, IM_ARRAYSIZE(sInputDisplayColorNames)))
            set->getSettings()->mInputDisplayButtonPressedColor = (SettingsMgr::InputDisplayColor)buttonPressedColor;

        if (ImGui::Combo("Stick Color", &stickColor, sInputDisplayColorNames, IM_ARRAYSIZE(sInputDisplayColorNames)))
            set->getSettings()->mInputDisplayStickColor = (SettingsMgr::InputDisplayColor)stickColor;

        if (ImGui::Combo("Ring Color", &ringColor, sInputDisplayColorNames, IM_ARRAYSIZE(sInputDisplayColorNames)))
            set->getSettings()->mInputDisplayRingColor = (SettingsMgr::InputDisplayColor)ringColor;

        if (ImGui::Combo("BG Color", &bgColor, sInputDisplayColorNames, IM_ARRAYSIZE(sInputDisplayColorNames)))
            set->getSettings()->mInputDisplayBackColor = (SettingsMgr::InputDisplayColor)bgColor;

        if (ImGui::InputInt("Position X", &posX, 5)) set->getSettings()->mInputDisplayPos.x = posX;
        if (ImGui::InputInt("Position Y", &posY, 5)) set->getSettings()->mInputDisplayPos.y = posY;
        ImGui::PopItemWidth();
        if (ImGui::Button("Reset Position")) {
            set->getSettings()->mInputDisplayPos = ImVec2(1600.f / 2, 900.f / 2);
            posX = set->getSettings()->mInputDisplayPos.x;
            posY = set->getSettings()->mInputDisplayPos.y;
        }
        ImGui::Unindent();
    }
    drawHotkeysCat();
    drawInfoCat();

    ImGui::End();
}

void Menu::handleAlways() {
    set = SettingsMgr::instance();
    gameSeq = (HakoniwaSequence*)GameSystemFunction::getGameSystem()->mSequence;
    stageScene = helpers::tryGetStageScene(gameSeq);
    // BAAAADD
    if (stageScene) {
        if (strcmp(GameDataFunction::getCurrentStageName(GameDataHolderAccessor(stageScene)), "ClashWorldHomeStage") == 0) noGetPlayer = false;
    }
    if (!noGetPlayer) {
        player = helpers::tryGetPlayerActor();
        playerHak = helpers::tryGetPlayerActorHakoniwa();
    }
    holder = helpers::tryGetGameDataHolder();

    if (InputHelper::isPressStickL() && mIsEnabledMenu) {
        prevNavId = GImGui->NavId;
        mIsEnabledMenu = false;
    } else if (InputHelper::isPressStickL() && !mIsEnabledMenu) {
        mIsEnabledMenu = true;
        prevTime = globalTimer;
    }
    if (globalTimer - prevTime < 5) {
        if (prevNavId) ImGui::SetFocusID(prevNavId, ImGui::FindWindowByName("SMOO+ Panel"));
        GImGui->NavDisableHighlight = false;
    }

    if (InputHelper::isPressPadLeft() && set->getSettings()->mIsEnableTpHotkeys && (!InputHelper::isInputToggled() || !mIsEnabledMenu)) {
        saveTeleport(tpStates[tpIndex]);
    } else if (InputHelper::isPressPadRight() && set->getSettings()->mIsEnableTpHotkeys && (!InputHelper::isInputToggled() || !mIsEnabledMenu)) {
        loadTeleport(tpStates[tpIndex]);
    }

    if (isHotkey(set->getSettings()->mKillSceneKey)) {
        if (stageScene) stageScene->kill();
    }
    if (isAnimHotkey(2)) {
        playerHak->endDemoPuppetable();
    }

     if (isAnimHotkey(4)) {
        playerHak->startDemoPuppetable();
        if(playerHak)playerHak->mAnimator->startAnim(Animation[set->mSettings.mPlayAnim]);

    }

    drawInputDisplay();
}

void Menu::setupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.IndentSpacing = 10;

    // Change the color of the title bar
    colors[ImGuiCol_TitleBg] = ImVec4(0.73f, 0.34f, 0.4f, 1.f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.73f, 0.34f, 0.4f, 1.f);

    // Change the color of the frame background
    colors[ImGuiCol_FrameBg] = ImVec4(0.73f, 0.34f, 0.4f, 1.f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.83f, 0.44f, 0.5f, 1.f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.73f, 0.34f, 0.4f, 1.f);

    // Change the color of the button
    colors[ImGuiCol_Button] = ImVec4(0.73f, 0.34f, 0.4f, 1.f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.83f, 0.44f, 0.5f, 1.f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.73f, 0.34f, 0.4f, 1.f);

    colors[ImGuiCol_Header] = ImVec4(0.73f, 0.34f, 0.4f, 1.f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.83f, 0.44f, 0.5f, 1.f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.73f, 0.34f, 0.4f, 1.f);

    colors[ImGuiCol_NavHighlight] = ImVec4(1, 1, 1, 1);
}

void Menu::drawInputDisabled() {
    ImGui::Begin(
        "Input Disabled", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
    );
    ImGui::SetWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetWindowPos(ImVec2(mWindowSize.x + 10, 0), ImGuiCond_FirstUseEver);
    ImGui::SetWindowFontScale(2);
    ImGui::Text("Input Disabled");
    ImGui::End();
}

void Menu::drawTeleportCat() {
    if (ImGui::CollapsingHeader("Teleport")) {
        ImGui::Indent();
        ImGui::PushItemWidth(200);
        ImGui::InputInt("Teleport Index", &tpIndex);
        if (tpIndex < 0) tpIndex = hk::util::arraySize(tpStates) - 1;
        if (tpIndex >= hk::util::arraySize(tpStates)) tpIndex = 0;
        ImGui::PopItemWidth();

        if (ImGui::Button("Save")) saveTeleport(tpStates[tpIndex]);
        ImGui::SameLine();
        if (ImGui::Button("Load")) loadTeleport(tpStates[tpIndex]);
        ImGui::SameLine();
        ImGui::PushID("TpHotkeys");
        ImGui::Checkbox("Hotkeys", &set->getSettings()->mIsEnableTpHotkeys);
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::BeginDisabled();
        ImGui::Checkbox("Saved", &tpStates[tpIndex].saved);
        ImGui::Text("Stage: %s", tpStates[tpIndex].stageName);
        ImGui::EndDisabled();

        if (ImGui::Button("Save To File")) {
            SaveFileHelper::instance()->saveTeleport(tpStates, hk::util::arraySize(tpStates));
        }

        ImGui::SameLine();
        if (ImGui::Button("Load From File")) {
            SaveFileHelper::instance()->loadTeleport(tpStates, hk::util::arraySize(tpStates));
        }
        ImGui::Unindent();
    }
}

void Menu::saveTeleport(TpState& state) {
    if (!stageScene || !playerHak) return;

    state.saved = true;
    state.pos = al::getTrans(playerHak);
    state.quat = al::getQuat(playerHak);
    strcpy(state.stageName, getEnglishName(GameDataFunction::getCurrentStageName(GameDataHolderAccessor(stageScene))));
}

void Menu::loadTeleport(TpState& state) {
    if (!stageScene || !playerHak) return;
    al::LiveActor* hack = playerHak->mHackKeeper->mCurrentHackActor;

    if (hack) {
        al::setTrans(hack, state.pos);
        al::updatePoseQuat(hack, state.quat);
        al::setVelocityZero(hack);
        return;
    }

    if (set->getSettings()->mIsEnableDisableTpPuppet && helpers::isGetShineState(stageScene)) {
        al::setTrans(playerHak, state.pos);
        al::updatePoseQuat(playerHak, state.quat);
    } else {
        playerHak->startDemoPuppetable();
        al::setTrans(playerHak, state.pos);
        al::updatePoseQuat(playerHak, state.quat);
        playerHak->endDemoPuppetable();
    }
}

void Menu::drawMiscCat() {
    if (ImGui::Button("Kill Mario")) {
        if (playerHak) GameDataFunction::killPlayer(GameDataHolderWriter(playerHak));
    }
    ImGui::SameLine();
    if (ImGui::Button("Damage Mario")) {
        if (playerHak) {
            bool tmpDamage = SettingsMgr::instance()->getSettings()->mIsEnableNoDamage;
            SettingsMgr::instance()->getSettings()->mIsEnableNoDamage = false;
            GameDataFunction::damagePlayer(GameDataHolderWriter(playerHak));
            SettingsMgr::instance()->getSettings()->mIsEnableNoDamage = tmpDamage;
        }
    }
    if (ImGui::Button("Life Up Heart")) {
        if (playerHak) GameDataFunction::getLifeMaxUpItem(playerHak);
    }
    ImGui::SameLine();
    if (ImGui::Button("Heal Mario")) {
        if (playerHak) GameDataFunction::recoveryPlayer(playerHak);
    }
    if (ImGui::Button("Add 1000 coins")) {
        if (stageScene) GameDataFunction::addCoin(GameDataHolderWriter(stageScene), 1000);
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove 1000 coins")) {
        if (stageScene) {
            if (GameDataFunction::getCoinNum(GameDataHolderAccessor(stageScene)) >= 1000) GameDataFunction::addCoin(GameDataHolderWriter(stageScene), -1000);
        }
    }
    if (ImGui::Button("Remove Cappy")) {
        if (playerHak) GameDataFunction::disableCapByPlacement((al::LiveActor*)playerHak->mHackCap);
    }
    ImGui::PushItemWidth(200);
    ImGui::Combo("Moon Refresh Text", &set->getSettings()->mMoonRefreshText, MoonRefreshTexts, IM_ARRAYSIZE(MoonRefreshTexts));
    ImGui::PopItemWidth();
}

void Menu::drawHotkeysCat() {
    if (ImGui::CollapsingHeader("Hotkeys")) {
        ImGui::Indent();
        ImGui::Combo("Kill Scene", &set->mSettings.mKillSceneKey, Keys, IM_ARRAYSIZE(Keys));
        ImGui::Combo("Play Anim", &set->mSettings.mPlayAnim, Animation, IM_ARRAYSIZE(Animation));
        ImGui::Unindent();
    }
}

bool Menu::isHotkey(int& key) {
    bool Up = InputHelper::isPressPadUp();
    bool Down = InputHelper::isPressPadDown();
    bool L = InputHelper::isHoldL();
    bool R = InputHelper::isHoldR();
    bool ZL = InputHelper::isHoldZL();
    bool ZR = InputHelper::isHoldZR();
    if (key == 0) return 0;
    if (key == 1) return Up && !InputHelper::isInputToggled();
    if (key == 2) return Up && L && !R && !ZL && !ZR && !InputHelper::isInputToggled();
    if (key == 3) return Up && R && !L && !ZL && !ZR && !InputHelper::isInputToggled(); 
    if (key == 4) return Up && ZL && !L && !R && !ZR && !InputHelper::isInputToggled();
    if (key == 5) return Up && ZR && !L && !R && !ZL && !InputHelper::isInputToggled();
    if (key == 6) return Up && L && R && !ZL && !ZR && !InputHelper::isInputToggled(); 
    if (key == 7) return Up && L && ZL && !R && !ZR && !InputHelper::isInputToggled();
    if (key == 8) return Up && L && ZR && !R && !ZL && !InputHelper::isInputToggled();
    if (key == 9) return Up && R && ZL && !L && !ZR && !InputHelper::isInputToggled();
    if (key == 10) return Up && R && ZR && !L && !ZL && !InputHelper::isInputToggled();
    return 0;
}

bool Menu::isAnimHotkey(int key) {
    bool Up = InputHelper::isPressPadUp();
    bool Down = InputHelper::isPressPadDown();
    bool L = InputHelper::isHoldL();
    bool R = InputHelper::isHoldR();
    bool ZL = InputHelper::isHoldZL();
    bool ZR = InputHelper::isHoldZR();
    if (key == 0) return 0;
    if (key == 1) return Up && !InputHelper::isInputToggled();
    if (key == 2) return Up && L && !R && !ZL && !ZR && !InputHelper::isInputToggled();
    if (key == 3) return Up && R && !L && !ZL && !ZR && !InputHelper::isInputToggled(); 
    if (key == 4) return Up && !ZL && !L && !Down && R && !ZR && !InputHelper::isInputToggled();
    if (key == 5) return Up && ZR && !L && !R && !ZL && !InputHelper::isInputToggled();
    if (key == 6) return Up && L && R && !ZL && !ZR && !InputHelper::isInputToggled(); 
    if (key == 7) return Up && L && ZL && !R && !ZR && !InputHelper::isInputToggled();
    if (key == 8) return Up && L && ZR && !R && !ZL && !InputHelper::isInputToggled();
    if (key == 9) return Up && R && ZL && !L && !ZR && !InputHelper::isInputToggled();
    if (key == 10) return Up && R && ZR && !L && !ZL && !InputHelper::isInputToggled();
    return 0;
}

void Menu::drawInfoCat() {
    if (ImGui::CollapsingHeader("Info")) {
        ImGui::Indent();

        GameDataHolderAccessor* accessor = helpers::tryGetGameDataHolderAccess();

        if (!holder || !accessor) {
            ImGui::Text("No Game Data Holder");
            ImGui::Unindent();
            return;
        }
        s32 jumpCount = rs::getPlayerJumpCount(holder);
        s32 throwCapCount = rs::getPlayerThrowCapCount(holder);
        u64 playTimeTotal = GameDataFunction::getPlayTimeTotal(*accessor);
        u64 playTimeAcrossFile = GameDataFunction::getPlayTimeAcrossFile(*accessor);
        s32 totalCoinNum = rs::getTotalCoinNum(holder);
        ImGui::Text("Enable Emotes: R + UP");
        ImGui::Text("Disable Emotes: L + UP");
        ImGui::Text("PlaceHolder: ");
        ImGui::Text("PlaceHolder: ");
        ImGui::Text("PlaceHolder: ");
        ImGui::Unindent();
    }
}

const char* Menu::getMoonRefreshText() {
    if (strcmp(MoonRefreshTexts[set->getSettings()->mMoonRefreshText], "<blank>") != 0) {
        return MoonRefreshTexts[set->getSettings()->mMoonRefreshText];
    } else {
        return "";
    }
}

} // namespace btt
