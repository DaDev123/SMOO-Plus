#pragma once
#include "hk/gfx/ImGuiBackendNvn.h"

#include "nn/hid.h"

#include "sead/heap/seadExpHeap.h"

#include "al/Library/Memory/HeapUtil.h"

#include <cstring>
#include <utility>

#include "fsHelper.h"
#include "heap/seadHeap.h"
#include "imgui.h"

namespace imgui {

static sead::Heap* sImGuiHeap = nullptr;

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

    ImFont* font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(loadData.buffer, loadData.bufSize, 17.0f, &c, ranges.Data);

    hk::gfx::ImGuiBackendNvn::instance()->initTexture(false);
    ImGui::GetIO().FontDefault = font;
}

static void updateImGuiInput() {
    static nn::hid::MouseState mouseState{};
    static nn::hid::MouseState lastMouseState{};
    static nn::hid::KeyboardState keyboardState{};
    static nn::hid::KeyboardState lastKeyboardState{};

    lastMouseState = mouseState;
    lastKeyboardState = keyboardState;

    nn::hid::GetMouseState(&mouseState);
    nn::hid::GetKeyboardState(&keyboardState);

    ImGuiIO& io = ImGui::GetIO();

    // Mouse position
    io.AddMousePosEvent(mouseState.mX / 1280.f * io.DisplaySize.x, mouseState.mY / 720.f * io.DisplaySize.y);

    // Mouse buttons
    constexpr std::pair<nn::hid::MouseButton, ImGuiMouseButton> buttonMap[] = {{nn::hid::MouseButton::Left, ImGuiMouseButton_Left},
                                                                               {nn::hid::MouseButton::Right, ImGuiMouseButton_Right},
                                                                               {nn::hid::MouseButton::Middle, ImGuiMouseButton_Middle}};

    for (auto&& [hidBtn, imguiBtn] : buttonMap) {
        bool was = lastMouseState.mButtons.Test(int(hidBtn));
        bool now = mouseState.mButtons.Test(int(hidBtn));

        if (was != now)
            io.AddMouseButtonEvent(imguiBtn, now);
    }

    // Mouse wheel
    if (mouseState.mWheelDeltaX != 0)
        io.AddMouseWheelEvent(mouseState.mWheelDeltaX > 0 ? 1.0f : -1.0f, 0.0f);

    if (mouseState.mWheelDeltaY != 0)
        io.AddMouseWheelEvent(0.0f, mouseState.mWheelDeltaY > 0 ? 1.0f : -1.0f);

    // ----------------------------
    // Keyboard → ImGui
    // ----------------------------

    constexpr std::pair<nn::hid::KeyboardKey, ImGuiKey> keyMap[] = {

        // Letters
        {nn::hid::KeyboardKey::A, ImGuiKey_A},
        {nn::hid::KeyboardKey::B, ImGuiKey_B},
        {nn::hid::KeyboardKey::C, ImGuiKey_C},
        {nn::hid::KeyboardKey::D, ImGuiKey_D},
        {nn::hid::KeyboardKey::E, ImGuiKey_E},
        {nn::hid::KeyboardKey::F, ImGuiKey_F},
        {nn::hid::KeyboardKey::G, ImGuiKey_G},
        {nn::hid::KeyboardKey::H, ImGuiKey_H},
        {nn::hid::KeyboardKey::I, ImGuiKey_I},
        {nn::hid::KeyboardKey::J, ImGuiKey_J},
        {nn::hid::KeyboardKey::K, ImGuiKey_K},
        {nn::hid::KeyboardKey::L, ImGuiKey_L},
        {nn::hid::KeyboardKey::M, ImGuiKey_M},
        {nn::hid::KeyboardKey::N, ImGuiKey_N},
        {nn::hid::KeyboardKey::O, ImGuiKey_O},
        {nn::hid::KeyboardKey::P, ImGuiKey_P},
        {nn::hid::KeyboardKey::Q, ImGuiKey_Q},
        {nn::hid::KeyboardKey::R, ImGuiKey_R},
        {nn::hid::KeyboardKey::S, ImGuiKey_S},
        {nn::hid::KeyboardKey::T, ImGuiKey_T},
        {nn::hid::KeyboardKey::U, ImGuiKey_U},
        {nn::hid::KeyboardKey::V, ImGuiKey_V},
        {nn::hid::KeyboardKey::W, ImGuiKey_W},
        {nn::hid::KeyboardKey::X, ImGuiKey_X},
        {nn::hid::KeyboardKey::Y, ImGuiKey_Y},
        {nn::hid::KeyboardKey::Z, ImGuiKey_Z},

        // Digits
        {nn::hid::KeyboardKey::D0, ImGuiKey_0},
        {nn::hid::KeyboardKey::D1, ImGuiKey_1},
        {nn::hid::KeyboardKey::D2, ImGuiKey_2},
        {nn::hid::KeyboardKey::D3, ImGuiKey_3},
        {nn::hid::KeyboardKey::D4, ImGuiKey_4},
        {nn::hid::KeyboardKey::D5, ImGuiKey_5},
        {nn::hid::KeyboardKey::D6, ImGuiKey_6},
        {nn::hid::KeyboardKey::D7, ImGuiKey_7},
        {nn::hid::KeyboardKey::D8, ImGuiKey_8},
        {nn::hid::KeyboardKey::D9, ImGuiKey_9},

        // Navigation
        {nn::hid::KeyboardKey::LeftArrow, ImGuiKey_LeftArrow},
        {nn::hid::KeyboardKey::RightArrow, ImGuiKey_RightArrow},
        {nn::hid::KeyboardKey::UpArrow, ImGuiKey_UpArrow},
        {nn::hid::KeyboardKey::DownArrow, ImGuiKey_DownArrow},

        // Control keys
        {nn::hid::KeyboardKey::Space, ImGuiKey_Space},
        {nn::hid::KeyboardKey::Tab, ImGuiKey_Tab},
        {nn::hid::KeyboardKey::Backspace, ImGuiKey_Backspace},
        {nn::hid::KeyboardKey::Return, ImGuiKey_Enter},
        {nn::hid::KeyboardKey::NumPadEnter, ImGuiKey_Enter}};

    for (auto&& [hidKey, imguiKey] : keyMap) {
        bool was = lastKeyboardState.mKeys.Test(int(hidKey));
        bool now = keyboardState.mKeys.Test(int(hidKey));

        if (was != now)
            io.AddKeyEvent(imguiKey, now);
    }

    // Modifiers (proper ImGui 1.89+ way)
    io.AddKeyEvent(ImGuiMod_Shift,
                   keyboardState.mKeys.Test(int(nn::hid::KeyboardKey::LeftShift)) || keyboardState.mKeys.Test(int(nn::hid::KeyboardKey::RightShift)));

    io.AddKeyEvent(ImGuiMod_Ctrl,
                   keyboardState.mKeys.Test(int(nn::hid::KeyboardKey::LeftControl)) || keyboardState.mKeys.Test(int(nn::hid::KeyboardKey::RightControl)));

    io.AddKeyEvent(ImGuiMod_Alt, keyboardState.mKeys.Test(int(nn::hid::KeyboardKey::LeftAlt)) || keyboardState.mKeys.Test(int(nn::hid::KeyboardKey::RightAlt)));

    // ----------------------------
    // Text Input (safe + correct)
    // ----------------------------

    for (auto&& [hidKey, imguiKey] : keyMap) {
        bool pressedNow = keyboardState.mKeys.Test(int(hidKey)) && !lastKeyboardState.mKeys.Test(int(hidKey));

        if (!pressedNow)
            continue;

        // Letters & digits -> UTF-8
        if (imguiKey >= ImGuiKey_A && imguiKey <= ImGuiKey_Z) {
            char c = char('a' + (imguiKey - ImGuiKey_A));

            // Shift → uppercase
            if (io.KeyShift)
                c = char(toupper(c));

            char utf8[2] = {c, 0};
            io.AddInputCharactersUTF8(utf8);
        }

        if (imguiKey >= ImGuiKey_0 && imguiKey <= ImGuiKey_9) {
            char c = char('0' + (imguiKey - ImGuiKey_0));
            char utf8[2] = {c, 0};
            io.AddInputCharactersUTF8(utf8);
        }
    }

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
