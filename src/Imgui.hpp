#pragma once

#include "hk/gfx/ImGuiBackendNvn.h"
#include "hk/mem/BssHeap.h"

#include <cstring>

#include "fsHelper.h"
#include "imgui.h"

namespace imgui {

static const float displayHeight = 720.f, displayWidth = 1280.f;

static void setupFont() {
    FsHelper::LoadData loadData = {.path = "content:/DebugData/Font/ChironHeiHK-Regular.ttf"};
    FsHelper::loadFileFromPath(loadData);

    ImVector<ImWchar> ranges;
    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesDefault());
    builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesJapanese());
    builder.AddText(""
                    "！、"
                    "äüöß"
                    "!\"§$%&/()=?´`^°#+-·.,;:_'*\\}][{<>|"
                    "←→↓↑");

    builder.BuildRanges(&ranges);

    ImFontConfig c{};
    strncpy(c.Name, "ChironHeiHK-Regular", sizeof(c.Name) - 1);

    ImFont* font =
        ImGui::GetIO().Fonts->AddFontFromMemoryTTF(loadData.buffer, loadData.bufSize, 17.0f, &c, ranges.Data);

    hk::gfx::ImGuiBackendNvn::instance()->initTexture(false);
    ImGui::GetIO().FontDefault = font;
}

static void setup() {
    hk::gfx::ImGuiBackendNvn* imgui = hk::gfx::ImGuiBackendNvn::instance();

    imgui->setAllocator({[](size allocSize, size alignment) -> void* {
                             return hk::mem::sMainHeap.allocate(allocSize, alignment);
                         },
                         [](void* ptr) -> void { hk::mem::sMainHeap.free(ptr); }});

    imgui->tryInitialize();
    setupFont();
}

}  // namespace imgui
