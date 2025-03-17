#pragma once

#include "imgui.h"
#include "settings/SettingsMgr.h"
#include <sead/basis/seadTypes.h>

namespace btt {

constexpr static ImVec4 sInputDisplayColors[] {
    { 0, 0, 0, 0 }, // None
    { 255, 255, 255, 255 }, // White
    { 128, 128, 128, 255 }, // Gray
    { 0, 0, 0, 255 }, // Black
    { 235, 64, 52, 255 }, // Red
    { 52, 58, 235, 255 }, // Blue
    { 235, 210, 52, 255 }, // Yellow
    { 58, 235, 52, 255 }, // Green
    { 235, 152, 52, 255 }, // Orange
    { 113, 52, 235, 255 }, // Purple
    { 225, 52, 235, 255 }, // Pink
    { 52, 235, 235, 255 } // Light Blue
};

constexpr const char* sInputDisplayColorNames[] {
    "None",
    "White",
    "Gray",
    "Black",
    "Red",
    "Blue",
    "Yellow",
    "Green",
    "Orange",
    "Purple",
    "Pink",
    "Light Blue"
};

inline ImVec4 getInputDisplayColor(SettingsMgr::InputDisplayColor color) { return sInputDisplayColors[int(color)]; }

    void drawInputDisplay();

} // namespace btt