#pragma once
#include "hk/gfx/ImGuiBackendNvn.h"

#include "al/Library/Memory/HeapUtil.h"

#include "sead/heap/seadExpHeap.h"

#include <cstring>
#include <vector>

#include "fsHelper.h"
#include "heap/seadHeap.h"
#include "imgui.h"
#include "nn/hid.h"

namespace imgui {
static sead::Heap* sImGuiHeap = nullptr;

static void setupFont() {
    FsHelper::LoadData loadData = {.path = "content:/DebugData/Font/ChironHeiHK-Regular.ttf"};

    FsHelper::loadFileFromPath(loadData);

    ImVector<ImWchar> ranges;
    ImFontGlyphRangesBuilder builder;
    builder.AddText(" "
                    "abcdefghikjlmnopqrstuvwxyz"
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                    "0123456789"
                    "あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよらりるれろわをんアイウエオカキク"
                    "ケコサシスセソタチツテトナニヌ"
                    "ネノハヒフヘホマミムメモヤユヨラリルレロワヲンぁぃぅぇぉっゃゅょァィゥェォッャュョーがぎぐげござじずぜぞだぢ"
                    "づでどばびぶべぼぱぴぷぺぽガギ"
                    "グゲゴザジズゼゾダヂヅデドバビブベボパピプペポ"
                    "日本語詳細設定配位置無有効化十字乱数次前行常時入力表示触瞬間認識切替旗飛中間記録"
                    "上下左右"
                    "！、"
                    "äüöß"
                    "!\"§$%&/()=?´`^°#+-·.,;:_'*\\}][{<>|"
                    "←→↓↑");
    builder.BuildRanges(&ranges);

    ImFontConfig c;
    strncpy(c.Name, "ChironHeiHK-Regular", sizeof(c.Name) - 1);
    c.Name[sizeof(c.Name) - 1] = '\0';

    ImFont* font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(loadData.buffer, loadData.bufSize, 17, &c, ranges.Data);

    hk::gfx::ImGuiBackendNvn::instance()->initTexture(false);
    ImGui::GetIO().FontDefault = font;
}

static void updateImGuiInput() {
    static nn::hid::MouseState state;
    static nn::hid::MouseState lastState;

    lastState = state;
    nn::hid::GetMouseState(&state);

    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(state.mX / 1280.f * io.DisplaySize.x, state.mY / 720.f * io.DisplaySize.y);
    constexpr std::pair<nn::hid::MouseButton, ImGuiMouseButton> buttonMap[] = {
        {nn::hid::MouseButton::Left, ImGuiMouseButton_Left},
        {nn::hid::MouseButton::Right, ImGuiMouseButton_Right},
        {nn::hid::MouseButton::Middle, ImGuiMouseButton_Middle},
    };

    for (const auto& [hidButton, imguiButton] : buttonMap) {
        if (state.mButtons.Test(int(hidButton)) && !lastState.mButtons.Test(int(hidButton))) {
            io.AddMouseButtonEvent(imguiButton, true);
        } else if (!state.mButtons.Test(int(hidButton)) && lastState.mButtons.Test(int(hidButton))) {
            io.AddMouseButtonEvent(imguiButton, false);
        }
    }

    if (state.mWheelDeltaX != 0.0f)
        io.AddMouseWheelEvent(0.0f, state.mWheelDeltaX > 0.0f ? 1.5f : -1.5f);

    /* Keyboard missing */

    io.MouseDrawCursor = true;
}

static void setup() {
    sImGuiHeap = sead::ExpHeap::create(2_MB, "ImGuiHeap", al::getStationedHeap(), 8, sead::Heap::cHeapDirection_Forward, false);
    hk::gfx::ImGuiBackendNvn* imgui = hk::gfx::ImGuiBackendNvn::instance();

    imgui->setAllocator(
        {[](size allocSize, size alignment) -> void* { return sImGuiHeap->tryAlloc(allocSize, alignment); }, [](void* ptr) -> void { sImGuiHeap->free(ptr); }});

    imgui->tryInitialize();
    setupFont();
}
}  // namespace imgui
