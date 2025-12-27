#include "imgui.h"
#include "Settings/SmooSettings.hpp"
#include "TwistsConfig.hpp"
#include "Settings/StageWarper.hpp"
#include "server/Client.hpp"

namespace SmooSettings {
void showSmooSettingsWindow(bool* p_open) {
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


    ImGui::NewLine();
    ImGui::End();
}
}
