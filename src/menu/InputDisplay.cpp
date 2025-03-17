#include "InputDisplay.h"
#include "helpers/InputHelper.h"
#include "imgui.h"
#include "settings/SettingsMgr.h"
#include <math/seadVector.h>

namespace btt {

using InputCallback = bool;

static ImU32 makeColor(const ImVec4& color) { return IM_COL32(u8(color.x), u8(color.y), u8(color.z), u8(color.w)); }

static void drawButton(const ImVec2& pos, InputCallback callback, float radius = 8, ImU32 color = makeColor(getInputDisplayColor(SettingsMgr::instance()->getSettings()->mInputDisplayButtonColor)), ImU32 pressedColor = makeColor(getInputDisplayColor(SettingsMgr::instance()->getSettings()->mInputDisplayButtonPressedColor)))
{
    const ImU32 col = callback ? pressedColor : color;
    ImGui::GetForegroundDrawList()->AddCircleFilled(pos, radius, col, 16);
}

static void drawButtonRect(const ImVec2& pos, InputCallback callback, ImU32 color = makeColor(getInputDisplayColor(SettingsMgr::instance()->getSettings()->mInputDisplayButtonColor)), ImU32 pressedColor = makeColor(getInputDisplayColor(SettingsMgr::instance()->getSettings()->mInputDisplayButtonPressedColor)))
{
    const ImU32 col = callback ? pressedColor : color;
    const ImVec2 min(pos.x - 13, pos.y - 6);
    const ImVec2 max(pos.x + 13, pos.y + 6);
    ImGui::GetForegroundDrawList()->AddRectFilled(min, max, col, 10.0f);
}

void drawInputDisplay()
{
    SettingsMgr* set = SettingsMgr::instance();
    if (!set->getSettings()->mIsEnableInputDisplay)
        return;

    const sead::Vector2f leftStick = {InputHelper::getLeftStickX(), InputHelper::getLeftStickY()};
    const sead::Vector2f rightStick = {InputHelper::getRightStickX(), InputHelper::getRightStickY()};

    ImVec2 pos = set->getSettings()->mInputDisplayPos;
    pos.x -= 200;
    pos.y -= 100;

    if (set->getSettings()->mInputDisplayBackColor != SettingsMgr::InputDisplayColor::None) {
        ImVec4 color = getInputDisplayColor(set->getSettings()->mInputDisplayBackColor);
        color.w = 128;
        ImGui::GetForegroundDrawList()->AddRectFilled({ pos.x - 50, pos.y - 100 }, { pos.x + 200, pos.y + 100 }, makeColor(color), 20.0f);
    }

    ImGui::GetForegroundDrawList()->AddCircle(pos, 25, makeColor(getInputDisplayColor(set->getSettings()->mInputDisplayRingColor)), 0, 2);
    ImVec2 leftPos = { pos.x + leftStick.x * 30, pos.y - leftStick.y * 30 };
    drawButton(leftPos,InputHelper::isHoldStickL(), 16, makeColor(getInputDisplayColor(set->getSettings()->mInputDisplayStickColor)));

    pos.x += 40;
    pos.y += 30;
    drawButton(pos, InputHelper::isHoldPadUp());
    pos.y += 30;
    drawButton(pos, InputHelper::isHoldPadDown());
    pos.x -= 15;
    pos.y -= 15;
    drawButton(pos, InputHelper::isHoldPadLeft());
    pos.x += 30;
    drawButton(pos, InputHelper::isHoldPadRight());

    pos.x += 60;
    ImGui::GetForegroundDrawList()->AddCircle(pos, 25, makeColor(getInputDisplayColor(set->getSettings()->mInputDisplayRingColor)), 0, 2);
    ImVec2 rightPos = { pos.x + rightStick.x * 30, pos.y - rightStick.y * 30 };
    drawButton(rightPos, InputHelper::isHoldStickR(), 16, makeColor(getInputDisplayColor(set->getSettings()->mInputDisplayStickColor)));

    pos.x += 40;
    pos.y -= 60;
    drawButton(pos, InputHelper::isHoldX());
    pos.y += 30;
    drawButton(pos, InputHelper::isHoldB());
    pos.x -= 15;
    pos.y -= 15;
    drawButton(pos, InputHelper::isHoldY());
    pos.x += 30;
    drawButton(pos, InputHelper::isHoldA());

    pos.y -= 10;
    pos.x -= 75;
    drawButton(pos, InputHelper::isHoldPlus(), 5);
    pos.x -= 40;
    drawButton(pos, InputHelper::isHoldMinus(), 5);

    pos = set->getSettings()->mInputDisplayPos;
    pos.x -= 200;
    pos.y -= 160;
    drawButtonRect(pos, InputHelper::isHoldL());
    pos.y -= 16;
    drawButtonRect(pos, InputHelper::isHoldZL());
    pos.x += 155;
    drawButtonRect(pos, InputHelper::isHoldZR());
    pos.y += 16;
    drawButtonRect(pos, InputHelper::isHoldR());
}

} // namespace pe