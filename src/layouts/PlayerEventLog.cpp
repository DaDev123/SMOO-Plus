#include "layouts/PlayerEventLog.h"

#include <basis/seadTypes.h>
#include <prim/seadSafeString.h>

#include "imgui.h"
#include "Library/Base/StringUtil.h"
#include "MessageMasterList.h"
#include "Scene/StageSceneStateModConfig.hpp"

PlayerEventLog* PlayerEventLog::sInstance = nullptr;
bool PlayerEventLog::mIsShow = true;

PlayerEventLog::PlayerEventLog() {}

PlayerEventLog::Entry::Entry() {
    mLife = 0;
}

PlayerEventLog::Entry::Entry(sead::FixedSafeString<16> player, Event event, sead::FixedSafeString<128> text) {
    mPlayer = player;
    mEvent = event;
    mText = text;
    mLife = calculateLife();
}

void PlayerEventLog::addEvent(sead::SafeString player, PlayerEventLog::Event event, sead::SafeString text) {
    // prevent duplicate entries for shines
    if (event == SHINE) {
        for (s32 i = 0; i < sNumEntries; i++) {
            if (mLog[i].mEvent == SHINE && al::isEqualString(mLog[i].mText, text))
                return;
        }
    }

    // if an entry already exists for purples, increase the number
    if (event == PURPLE) {
        for (s32 i = 0; i < sNumEntries; i++) {
            if (mLog[i].mEvent == PURPLE) {
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
    if (mIsShow) {
        f32 width = ImGui::GetIO().DisplaySize.x;
        f32 height = ImGui::GetIO().DisplaySize.y;
        ImGuiStyle& style = ImGui::GetStyle();
        ImFont* font = ImGui::GetFont();
        f32 charWidth = font->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, 0.0f, "X").x;
        f32 lineHeight = ImGui::GetTextLineHeightWithSpacing();
        f32 windowWidth =
            (charWidth * 128.0f) + (style.WindowPadding.x * 2.0f) + (style.FramePadding.x * 2.0f);
        f32 windowHeight =
            (lineHeight * 8.0f) + (style.WindowPadding.y * 2.0f);  // + ImGui::GetFrameHeight();
        ImGui::SetNextWindowPos(ImVec2(0, (height - windowHeight) * 0.5f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
        ImGui::Begin("Player Event Log", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoSavedSettings);

        for (s32 i = 0; i < sNumEntries; i++) {
            if (mLog[i].mLife != 0) {
                ImVec2 originalCursor = ImGui::GetCursorPos();
                switch (mLog[i].mEvent) {
                case CONNECT: {
                    ImGui::SetCursorPos(ImVec2(originalCursor.x + 1.0f, originalCursor.y + 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("%s connected", mLog[i].mPlayer.cstr());
                    ImGui::PopStyleColor();

                    ImGui::SetCursorPos(originalCursor);
                    ImGui::Text("%s connected", mLog[i].mPlayer.cstr());
                    break;
                }
                case DISCONNECT: {
                    ImGui::SetCursorPos(ImVec2(originalCursor.x + 1.0f, originalCursor.y + 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("%s disconnected", mLog[i].mPlayer.cstr());
                    ImGui::PopStyleColor();

                    ImGui::SetCursorPos(originalCursor);
                    ImGui::Text("%s disconnected", mLog[i].mPlayer.cstr());
                    break;
                }
                case START: {
                    ImGui::SetCursorPos(ImVec2(originalCursor.x + 1.0f, originalCursor.y + 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("%s started", mLog[i].mPlayer.cstr());
                    ImGui::PopStyleColor();

                    ImGui::SetCursorPos(originalCursor);
                    ImGui::Text("%s started", mLog[i].mPlayer.cstr());
                    break;
                }
                case SHINE: {
                    ImGui::SetCursorPos(ImVec2(originalCursor.x + 1.0f, originalCursor.y + 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("%s ", mLog[i].mPlayer.cstr());
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text("got the moon ");
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text("%s", mLog[i].mText.cstr());
                    ImGui::SameLine(0.0f, 0.0f);
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
                }
                case PURPLE: {
                    const char* plural = mLog[i].mNumPurples > 1 ? "s" : "";

                    ImGui::SetCursorPos(ImVec2(originalCursor.x + 1.0f, originalCursor.y + 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("%s ", mLog[i].mPlayer.cstr());
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text("got ");
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text("%d %s ", mLog[i].mNumPurples, mLog[i].mText.cstr());
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text("regional coin%s", plural);
                    ImGui::PopStyleColor();

                    ImGui::SetCursorPos(originalCursor);
                    ImGui::Text("%s ", mLog[i].mPlayer.cstr());

                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                    ImGui::Text("got ");
                    ImGui::PopStyleColor();

                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.0f, 1.0f, 1.0f));
                    ImGui::Text("%d %s ", mLog[i].mNumPurples, mLog[i].mText.cstr());
                    ImGui::PopStyleColor();

                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                    ImGui::Text("regional coin%s", plural);
                    ImGui::PopStyleColor();
                    break;
                }
                case CHECKPOINT: {
                    ImGui::SetCursorPos(ImVec2(originalCursor.x + 1.0f, originalCursor.y + 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("%s ", mLog[i].mPlayer.cstr());
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text("got the checkpoint ");
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text("%s", mLog[i].mText.cstr());
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
                case MOONROCK: {
                    ImGui::SetCursorPos(ImVec2(originalCursor.x + 1.0f, originalCursor.y + 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("%s ", mLog[i].mPlayer.cstr());
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text("hit the moon rock in ");
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text("%s", mLog[i].mText.cstr());
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::PopStyleColor();

                    ImGui::SetCursorPos(originalCursor);
                    ImGui::Text("%s ", mLog[i].mPlayer.cstr());

                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                    ImGui::Text("hit the moon rock in ");
                    ImGui::PopStyleColor();

                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.5f, 1.0f, 1.0f));
                    ImGui::Text("%s", mLog[i].mText.cstr());
                    ImGui::PopStyleColor();
                    break;
                }
                }
            }
        }
        ImGui::End();
    }

    // tick down life even when hidden
    for (s32 i = 0; i < sNumEntries; i++) {
        if (mLog[i].mLife > 0)
            mLog[i].mLife--;
    }
}

s32 PlayerEventLog::calculateLife() {
    switch (StageSceneStateModConfig::getSpeedrunLogLife()) {
    case StageSceneStateModConfig::INF:
        return -1;
    case StageSceneStateModConfig::FIFTEEN:
        return 900;
    case StageSceneStateModConfig::TEN:
        return 600;
    case StageSceneStateModConfig::FIVE:
        return 300;
    }
}

const char* PlayerEventLog::getShineMessage(sead::SafeString stage, sead::SafeString objId) {
    for (const MessageMasterList::MessageData& data : MessageMasterList::shineList) {
        if (al::isEqualString(stage, data.stage) && al::isEqualString(objId, data.objId)) {
            return data.text;
        }
    }
    return "NULL";
}

const char* PlayerEventLog::getCheckpointMessage(sead::SafeString objId) {
    for (const MessageMasterList::MessageData& data : MessageMasterList::checkpointList) {
        if (al::isEqualString(objId, data.objId)) {
            return data.text;
        }
    }
    return "NULL";
}

const char* PlayerEventLog::getAchievementMessage(sead::SafeString label) {
    for (const MessageMasterList::AchievementMessageData& data : MessageMasterList::achievementList) {
        if (al::isEqualString(label, data.label)) {
            return data.text;
        }
    }
    return "NULL";
}
