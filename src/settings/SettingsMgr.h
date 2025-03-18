#pragma once

#include <heap/seadDisposer.h>
#include "settings/SettingsHooks.h"
#include "imgui.h"

namespace btt {

#define SETTING(NAME) bool mIsEnable##NAME;

class SettingsMgr {
    SEAD_SINGLETON_DISPOSER(SettingsMgr)

private:
    sead::Heap* mHeap;
public:
    SettingsMgr() = default;

    enum class InputDisplayColor : u8 {
        None,
        White,
        Gray,
        Black,
        Red,
        Blue,
        Yellow,
        Green,
        Orange,
        Purple,
        Pink,
        LightBlue
    };

    struct Settings {
        SETTING(DisableMusic);
        SETTING(InputDisplay);
        InputDisplayColor mInputDisplayButtonColor = InputDisplayColor::White;
        InputDisplayColor mInputDisplayButtonPressedColor = InputDisplayColor::Pink;
        InputDisplayColor mInputDisplayStickColor = InputDisplayColor::White;
        InputDisplayColor mInputDisplayRingColor = InputDisplayColor::Gray;
        InputDisplayColor mInputDisplayBackColor = InputDisplayColor::Black;
        ImVec2 mInputDisplayPos = ImVec2(1600.f / 2, 900.f / 2);
        int mKillSceneKey = 0;
        int mPlayAnim = 0;
        int mHealMarioKey = 0;
        int mMoonRefreshText = 0;
        
    } mSettings;

    // Settings* mSettings = nullptr;
    Settings* getSettings() { return &mSettings; }
    char* mPath = "sd:/SMOO-Plus/Settings.bin";
    };



} // namespace btt