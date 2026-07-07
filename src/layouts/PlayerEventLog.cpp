#include "layouts/PlayerEventLog.h"

#include <basis/seadTypes.h>
#include <cstdarg>
#include <prim/seadSafeString.h>

#include "account.h"
#include "imgui.h"
#include "Library/Base/StringUtil.h"
#include "MessageMasterList.h"
#include "puppets/PuppetInfo.h"
#include "Scene/StageSceneStateModConfig.hpp"
#include "server/Client.hpp"

SEAD_SINGLETON_DISPOSER_IMPL(PlayerEventLog)
bool PlayerEventLog::mIsShow = true;

PlayerEventLog::PlayerEventLog() {}

PlayerEventLog::Entry::Entry() {
    mLife = 0;
}

PlayerEventLog::Entry::Entry(nn::account::Uid player, Event event, sead::FixedSafeString<128> text) {
    mPlayerId = player;
    mPlayerName = "???";
    mEvent = event;
    mText = text;
    mLife = calculateLife();
}

PlayerEventLog::Entry::Entry(Event event, sead::FixedSafeString<128> text) {
    mPlayerName = "You";
    mIsPlayerName = true;
    mEvent = event;
    mText = text;
    mLife = calculateLife();
}

void PlayerEventLog::addEvent(nn::account::Uid player, PlayerEventLog::Event event, sead::SafeString text) {
    if (!sInstance)
        return;
    // prevent duplicate entries for shines
    if (event == SHINE) {
        for (s32 i = 0; i < sNumEntries; i++) {
            if (sInstance->mLog[i].mEvent == SHINE && al::isEqualString(sInstance->mLog[i].mText, text))
                return;
        }
    }

    // if an entry already exists for purples, increase the number
    if (event == PURPLE) {
        for (s32 i = 0; i < sNumEntries; i++) {
            if (sInstance->mLog[i].mEvent == PURPLE) {
                if (sInstance->mLog[i].mPlayerId == player &&
                    al::isEqualString(sInstance->mLog[i].mText, text)) {
                    s32 numPurples = sInstance->mLog[i].mNumPurples + 1;
                    for (s32 j = i; j > 0; j--) {
                        sInstance->mLog[j] = sInstance->mLog[j - 1];
                    }
                    sInstance->mLog[0] = Entry(player, event, text);
                    sInstance->mLog[0].mNumPurples = numPurples;
                    return;
                }
            }
        }
    }

    for (s32 i = sNumEntries - 1; i > 0; i--) {
        sInstance->mLog[i] = sInstance->mLog[i - 1];
    }

    sInstance->mLog[0] = Entry(player, event, text);
}

void PlayerEventLog::addSelfEvent(PlayerEventLog::Event event, sead::SafeString text) {
    if (!sInstance)
        return;

    // prevent duplicate entries for shines
    if (event == SHINE) {
        for (s32 i = 0; i < sNumEntries; i++) {
            if (sInstance->mLog[i].mEvent == SHINE && al::isEqualString(sInstance->mLog[i].mText, text))
                return;
        }
    }

    // if an entry already exists for purples, increase the number
    if (event == PURPLE) {
        for (s32 i = 0; i < sNumEntries; i++) {
            if (sInstance->mLog[i].mEvent == PURPLE) {
                if (sInstance->mLog[i].mPlayerName == "You" &&
                    al::isEqualString(sInstance->mLog[i].mText, text)) {
                    s32 numPurples = sInstance->mLog[i].mNumPurples + 1;
                    for (s32 j = i; j > 0; j--) {
                        sInstance->mLog[j] = sInstance->mLog[j - 1];
                    }
                    sInstance->mLog[0] = Entry(event, text);
                    sInstance->mLog[0].mNumPurples = numPurples;
                    return;
                }
            }
        }
    }

    for (s32 i = sNumEntries - 1; i > 0; i--) {
        sInstance->mLog[i] = sInstance->mLog[i - 1];
    }

    sInstance->mLog[0] = Entry(event, text);
}

void PlayerEventLog::update() {
    if (mIsShow) {
        tryUpdateNames();

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
                    ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), "%s connected",
                                       mLog[i].mPlayerName.cstr());

                    ImGui::SetCursorPos(originalCursor);
                    ImGui::Text("%s connected", mLog[i].mPlayerName.cstr());
                    break;
                }
                case DISCONNECT: {
                    ImGui::SetCursorPos(ImVec2(originalCursor.x + 1.0f, originalCursor.y + 1.0f));
                    ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), "%s disconnected",
                                       mLog[i].mPlayerName.cstr());

                    ImGui::SetCursorPos(originalCursor);
                    ImGui::Text("%s disconnected", mLog[i].mPlayerName.cstr());
                    break;
                }
                case START: {
                    ImGui::SetCursorPos(ImVec2(originalCursor.x + 1.0f, originalCursor.y + 1.0f));
                    ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), "%s started",
                                       mLog[i].mPlayerName.cstr());

                    ImGui::SetCursorPos(originalCursor);
                    ImGui::Text("%s started", mLog[i].mPlayerName.cstr());
                    break;
                }
                case SHINE: {
                    ImGui::SetCursorPos(ImVec2(originalCursor.x + 1.0f, originalCursor.y + 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("%s ", mLog[i].mPlayerName.cstr());
                    ImGui::SameLine(0, 0);
                    ImGui::Text("got the moon ");
                    ImGui::SameLine(0, 0);
                    ImGui::Text("%s", mLog[i].mText.cstr());
                    ImGui::SameLine(0, 0);
                    ImGui::PopStyleColor();

                    ImGui::SetCursorPos(originalCursor);
                    ImGui::Text("%s ", mLog[i].mPlayerName.cstr());

                    ImGui::SameLine(0, 0);
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "got the moon ");

                    ImGui::SameLine(0, 0);
                    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.0f, 1.0f), "%s", mLog[i].mText.cstr());
                    break;
                }
                case PURPLE: {
                    const char* plural = mLog[i].mNumPurples > 1 ? "s" : "";

                    ImGui::SetCursorPos(ImVec2(originalCursor.x + 1.0f, originalCursor.y + 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("%s ", mLog[i].mPlayerName.cstr());
                    ImGui::SameLine(0, 0);
                    ImGui::Text("got ");
                    ImGui::SameLine(0, 0);
                    ImGui::Text("%d %s ", mLog[i].mNumPurples, mLog[i].mText.cstr());
                    ImGui::SameLine(0, 0);
                    ImGui::Text("regional coin%s", plural);
                    ImGui::PopStyleColor();

                    ImGui::SetCursorPos(originalCursor);
                    ImGui::Text("%s ", mLog[i].mPlayerName.cstr());

                    ImGui::SameLine(0, 0);
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "got ");

                    ImGui::SameLine(0, 0);
                    ImGui::TextColored(ImVec4(0.85f, 0.0f, 1.0f, 1.0f), "%d %s ", mLog[i].mNumPurples,
                                       mLog[i].mText.cstr());

                    ImGui::SameLine(0, 0);
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "regional coin%s", plural);
                    break;
                }
                case CHECKPOINT: {
                    ImGui::SetCursorPos(ImVec2(originalCursor.x + 1.0f, originalCursor.y + 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("%s ", mLog[i].mPlayerName.cstr());
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text("got the checkpoint ");
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text("%s", mLog[i].mText.cstr());
                    ImGui::PopStyleColor();

                    ImGui::SetCursorPos(originalCursor);
                    ImGui::Text("%s ", mLog[i].mPlayerName.cstr());

                    ImGui::SameLine(0, 0);
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "got the checkpoint ");

                    ImGui::SameLine(0, 0);
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", mLog[i].mText.cstr());
                    break;
                }
                case MOONROCK: {
                    ImGui::SetCursorPos(ImVec2(originalCursor.x + 1.0f, originalCursor.y + 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("%s ", mLog[i].mPlayerName.cstr());
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text("hit the moon rock in ");
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text("%s", mLog[i].mText.cstr());
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::PopStyleColor();

                    ImGui::SetCursorPos(originalCursor);
                    ImGui::Text("%s ", mLog[i].mPlayerName.cstr());

                    ImGui::SameLine(0, 0);
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "hit the moon rock in ");

                    ImGui::SameLine(0, 0);
                    ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "%s", mLog[i].mText.cstr());
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

void PlayerEventLog::tryUpdateNames() {
    for (s32 i = 0; i < sNumEntries; i++) {
        if (mLog[i].mIsPlayerName)
            continue;

        PuppetInfo* player = Client::findPuppetInfo(mLog[i].mPlayerId, false);
        if (!player)
            continue;

        mLog[i].mPlayerName = player->puppetName;
        mLog[i].mIsPlayerName = true;
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
