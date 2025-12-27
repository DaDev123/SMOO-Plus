#include "Settings/StageWarper.hpp"
#include "nn/hid.h"
#include <imgui.h>
#include <algorithm>
#include "server/Client.hpp"
#include "game/Sequence/ChangeStageInfo.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolderAccessor.h"
#include "game/Scene/StageScene.h"
#include "game/System/GameSystem.h"
#include "server/gamemode/GameModeBase.hpp"
#include "al/Library/Sequence/Sequence.h"
#include "al/Library/Base/StringUtil.h"
#include "game/Sequence/HakoniwaSequence.h"


namespace StageWarper
{

    static std::string pendingWarpStage;


    bool IsOpen = false;

    static char searchBuf[200] = "";

    static const int g_stagesCount = sizeof(g_stages) / sizeof(g_stages[0]);

    static void DrawSearchResults()
{
    const char* query = searchBuf;

    ImGui::BeginChild("stage_results", ImVec2(0, 150), true); // kleineres Fenster

    if (query[0] != '\0')
    {
        int displayed = 0;
        for (int i = 0; i < g_stagesCount && displayed < 5; ++i)
        {
            const char* stage = g_stages[i];

            // Case-insensitive Match
            if (!strcasestr(stage, query))
                continue;

            if (ImGui::Selectable(stage))
            {
                warpPlayer(stage);
            }

            displayed++;
        }

        if (displayed == 0)
            ImGui::Text("No matches");
    }
    else
    {
        ImGui::Text("Type to search stages...");
    }

    ImGui::EndChild();
}



    void ShowSearchWindow()
    {
         if (!IsOpen)
                return;

        if (SmooSettings::HAS_KEYBOARD == true)
        {
            ImGui::Begin(
                "Stage Search",
                &IsOpen,
                ImGuiWindowFlags_AlwaysAutoResize
            );

            ImGui::InputTextWithHint(
                "##stage_search",
                "Stage name...",
                searchBuf,
                IM_ARRAYSIZE(searchBuf)
            );

        ImGui::Separator();

        DrawSearchResults();

        ImGui::End();
        }
    }

GameDataHolder* tryGetGameDataHolder()
{
    auto* sequence = GameSystemFunction::getGameSystem()->mSequence;
    if (!sequence)
        return nullptr;

    // Compare the sequence name, since Scene has no name in this SDK
    if (!al::isEqualString(sequence->mName.cstr(), "HakoniwaSequence"))
        return nullptr;

    auto* hakoniwa = static_cast<HakoniwaSequence*>(sequence);
    return hakoniwa->mGameDataHolderAccessor.mData;
}


void warpPlayer(const char* stageName)
{
    GameDataHolder* holder = tryGetGameDataHolder();
    if (!holder)
        return;

    GameDataHolderWriter writer(holder);

    ChangeStageInfo info(
        writer.mData,
        "",
        stageName,
        false,
        -1,
        ChangeStageInfo::SubScenarioType::NO_SUB_SCENARIO
    );

    GameDataFunction::tryChangeNextStage(writer, &info);
}


}  // namespace StageWarper
