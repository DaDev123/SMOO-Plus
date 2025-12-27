#include "imgui.h"
#include "Settings/SmooSettings.hpp"
#include "TwistsConfig.hpp"
#include "Settings/StageWarper.hpp"
#include "server/Client.hpp"

namespace SmooSettings {
    bool HAS_KEYBOARD = false;
    ImVec4 g_WindowColor = ImVec4(0.1f, 0.1f, 0.1f, 1.0f); // Standardfarbe
    bool showColorPicker = false;

    void ApplyGlobalWindowColor() {
        ImGuiStyle& style = ImGui::GetStyle();
        style.Colors[ImGuiCol_WindowBg] = g_WindowColor;
    }

    void ShowColorPickerWindow() {
        if (!showColorPicker) return;
        ImGui::Begin("Color Picker", &showColorPicker);
        ImGui::ColorEdit4("Window Background", (float*)&g_WindowColor); // RGBA  
        ImGui::ColorEdit4("Text Color", (float*)&ImGui::GetStyle().Colors[ImGuiCol_Text]);
        ImGui::ColorEdit4("Button Color", (float*)&ImGui::GetStyle().Colors[ImGuiCol_Button]);
        ImGui::ColorEdit4("Button Hovered Color", (float*)&ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
        ImGui::ColorEdit4("Button Active Color", (float*)&ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
        ImGui::ColorEdit4("Frame Background", (float*)&ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
        ImGui::ColorEdit4("Frame Background Hovered", (float*)&ImGui::GetStyle().Colors[ImGuiCol_FrameBgHovered]);
        ImGui::ColorEdit4("Frame Background Active", (float*)&ImGui::GetStyle().Colors[ImGuiCol_FrameBgActive]);
        ImGui::ColorEdit4("Title Background", (float*)&ImGui::GetStyle().Colors[ImGuiCol_TitleBg]);
        ImGui::ColorEdit4("Title Background Active", (float*)&ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive]);

        ImGui::End();
    }

    void showSmooSettingsWindow(bool* p_open) {
        // Zuerst globalen Style anwenden
        ApplyGlobalWindowColor();

        // Settings Fenster
        ImGui::Begin("SMOO+ Settings", p_open, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("SMOO+ Settings will go here.");
        ImGui::Separator();
        ImGui::NewLine();

        // Twist Toggles
        ImGui::Text("Twist Toggles:");
        if (ImGui::Button("Ice Physics Toggle")) TwistsConfig::toggleIcePhysics();
        if (ImGui::Button("Cappy Disable Toggle")) TwistsConfig::toggleCappyDisable();

        // Stage Settings
        if (ImGui::Button("Stage Search")) {
            StageWarper::IsOpen = true;
        }
        // Color Picker Toggle
        if (ImGui::Button("Color Picker")) {
            showColorPicker = !showColorPicker;
        }
        //
        ImGui::Checkbox("Enable Keyboard Input", &HAS_KEYBOARD);

        ImGui::NewLine();
        ImGui::End();

        // Color Picker separat rendern
        ShowColorPickerWindow();
    }
}
