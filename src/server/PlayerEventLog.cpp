#include "server/PlayerEventLog.h"

#include <basis/seadTypes.h>

#include "../Imgui.hpp"
#include "imgui.h"
#include "Library/Base/StringUtil.h"
#include "Library/Message/IUseMessageSystem.h"
#include "Library/Message/MessageSystem.h"
#include "logger.hpp"
#include "prim/seadSafeString.h"

PlayerEventLog* PlayerEventLog::sInstance = nullptr;

PlayerEventLog::PlayerEventLog(al::MessageSystem* messageSystem) {
    mMessageSystem = messageSystem;
}

PlayerEventLog::Entry::Entry() {
    mLife = 0;
}

PlayerEventLog::Entry::Entry(const char* player, Event event, sead::FixedSafeString<128> text) {
    mPlayer = player;
    mEvent = event;
    mText = text;
}

void PlayerEventLog::addEvent(const char* player, PlayerEventLog::Event event, sead::FixedSafeString<128> text) {
    if (event == purple) {
        for (s32 i = 0; i < sNumEntries; i++) {
            if (mLog[i].mEvent == purple) {
                if (al::isEqualString(mLog[i].mPlayer, player) && al::isEqualString(mLog[i].mText, text)) {
                    s32 numPurples = mLog[i].mNumPurples + 1;
                    for (s32 j = i; j > 0; j--) {
                        mLog[j] = mLog[j - 1];
                    }
                    mLog[0] = Entry(player, event, text);
                    mLog[0].mNumPurples = numPurples;
                    return;
                }
            }
        }
    }

    for (s32 i = sNumEntries - 1; i > 0; i--) {
        mLog[i] = mLog[i - 1];
    }

    mLog[0] = Entry(player, event, text);
}

void PlayerEventLog::update() {
    f32 width = ImGui::GetIO().DisplaySize.x;
    f32 height = ImGui::GetIO().DisplaySize.y;
    ImGuiStyle& style = ImGui::GetStyle();
    ImFont* font = ImGui::GetFont();
    f32 charWidth = font->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, 0.0f, "X").x;
    f32 lineHeight = ImGui::GetTextLineHeightWithSpacing();
    f32 windowWidth = (charWidth * 128.0f) + (style.WindowPadding.x * 2.0f) + (style.FramePadding.x * 2.0f);
    f32 windowHeight = (lineHeight * 8.0f) + (style.WindowPadding.y * 2.0f);  // + ImGui::GetFrameHeight();
    ImGui::SetNextWindowPos(ImVec2(0, (height - windowHeight) * 0.5f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
    ImGui::Begin("Player Event Log", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);

    for (s32 i = 0; i < sNumEntries; i++) {
        if (mLog[i].mLife > 0) {
            ImVec2 originalCursor = ImGui::GetCursorPos();
            switch (mLog[i].mEvent) {
            case connect:
                ImGui::SetCursorPos(ImVec2(originalCursor.x + 2.0f, originalCursor.y + 2.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("%s connected", mLog[i].mPlayer.cstr());
                ImGui::PopStyleColor();

                ImGui::SetCursorPos(originalCursor);
                ImGui::Text("%s connected", mLog[i].mPlayer.cstr());
                break;
            case disconnect:
                ImGui::SetCursorPos(ImVec2(originalCursor.x + 2.0f, originalCursor.y + 2.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("%s disconnected", mLog[i].mPlayer.cstr());
                ImGui::PopStyleColor();

                ImGui::SetCursorPos(originalCursor);
                ImGui::Text("%s disconnected", mLog[i].mPlayer.cstr());
                break;
            case shine:
                ImGui::SetCursorPos(ImVec2(originalCursor.x + 2.0f, originalCursor.y + 2.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("%s got the moon %s", mLog[i].mPlayer.cstr(), mLog[i].mText.cstr());
                ImGui::PopStyleColor();

                ImGui::SetCursorPos(originalCursor);
                ImGui::Text("%s ", mLog[i].mPlayer.cstr());

                ImGui::SameLine(0.0f, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                ImGui::Text("got the moon ");
                ImGui::PopStyleColor();

                ImGui::SameLine(0.0f, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.0f, 1.0f));
                ImGui::Text("%s", mLog[i].mText.cstr());
                ImGui::PopStyleColor();
                break;
            case purple:
                ImGui::SetCursorPos(ImVec2(originalCursor.x + 2.0f, originalCursor.y + 2.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("%s got %d %s purples", mLog[i].mPlayer.cstr(), mLog[i].mNumPurples, mLog[i].mText.cstr());
                ImGui::PopStyleColor();

                ImGui::SetCursorPos(originalCursor);
                ImGui::Text("%s ", mLog[i].mPlayer.cstr());

                ImGui::SameLine(0.0f, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                ImGui::Text("got ");
                ImGui::PopStyleColor();

                ImGui::SameLine(0.0f, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.0f, 1.0f, 1.0f));
                ImGui::Text("%d %s purples", mLog[i].mNumPurples, mLog[i].mText.cstr());
                ImGui::PopStyleColor();
                break;
            case checkpoint:
                ImGui::SetCursorPos(ImVec2(originalCursor.x + 2.0f, originalCursor.y + 2.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("%s got the checkpoint %s", mLog[i].mPlayer.cstr(), mLog[i].mText.cstr());
                ImGui::PopStyleColor();

                ImGui::SetCursorPos(originalCursor);
                ImGui::Text("%s ", mLog[i].mPlayer.cstr());

                ImGui::SameLine(0.0f, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                ImGui::Text("got the checkpoint ");
                ImGui::PopStyleColor();

                ImGui::SameLine(0.0f, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("%s", mLog[i].mText.cstr());
                ImGui::PopStyleColor();
                break;
            }
            // mLog[i].mLife--;
        }
    }

    ImGui::End();
}

void PlayerEventLog::kill() {
    sInstance = nullptr;
    delete this;
}
