/**
 * @file smallMarioHooks.hpp
 * @brief Hooks for Small Mario functionality — all toggleable at runtime
 */

#pragma once

#include "hk/hook/a64/Assembler.h"
#include "hk/hook/InstrUtil.h"
#include "hk/hook/Trampoline.h"
#include "hk/ro/RoUtil.h"
#include "hk/types.h"
#include "hk/util/Math.h"

#include "sead/math/seadVector.h"

#include "al/Library/Base/StringUtil.h"
#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Effect/EffectKeeper.h"
#include "al/Library/Effect/EffectSystemInfo.h"
#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorInitInfo.h"
#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/LiveActor/ActorSensorUtil.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/Player/PlayerUtil.h"
#include "al/Library/Scene/SceneUtil.h"
#include "al/Library/Yaml/ByamlUtil.h"
#include "al/Project/Action/ActionEffectCtrl.h"

#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Util/PlayerUtil.h"

#include "Imgui.hpp"
#include "Project/HitSensor/HitSensor.h"
#include "TwistsConfig.hpp"

namespace smallMario {
static bool sPlayerIs2D = false;
}

static constexpr float scale = 0.3f;

static HkTrampoline<void, al::ActionEffectCtrl*, const char*> effectHook =
    hk::hook::trampoline([](al::ActionEffectCtrl* effectController, const char* effectName) -> void {
        if (!TwistsConfig::isSmallMarioEnabled()) {
            effectHook.orig(effectController, effectName);
            return;
        }
        if (al::isEqualString(effectName, "RollingStart") || al::isEqualString(effectName, "Rolling") || al::isEqualString(effectName, "RollingStandUp") ||
            al::isEqualString(effectName, "Jump") || al::isEqualString(effectName, "LandDownFall") || al::isEqualString(effectName, "SpinCapStart") ||
            al::isEqualString(effectName, "FlyingWaitR") || al::isEqualString(effectName, "StayR") || al::isEqualString(effectName, "SpinGroundR") ||
            al::isEqualString(effectName, "StartSpinJumpR") || al::isEqualString(effectName, "SpinJumpDownFallR") || al::isEqualString(effectName, "Move") ||
            al::isEqualString(effectName, "Brake")) {
            return;
        }
        effectHook.orig(effectController, effectName);
    });

inline void sensorHook(al::LiveActor* actor, al::ActorInitInfo const& initInfo, char const* sensorName, u32 typeEnum, float radius, u16 maxCount,
                       sead::Vector3f const& position) {
    sead::Vector3f newPos = sead::Vector3f(position);
    if (TwistsConfig::isSmallMarioEnabled() && position.y > 0)
        newPos.y = position.y * 0.2f;
    al::addHitSensor(actor, initInfo, sensorName, typeEnum, radius, maxCount, newPos);
}

inline float followDistHook() {
    static bool toggled = false;
    static bool wasComboPressed = false;

    bool comboPressed = al::isPadHoldL(-1) && al::isPadTriggerUp(-1);
    if (comboPressed && !wasComboPressed)
        toggled = !toggled;
    wasComboPressed = comboPressed;

    if (smallMario::sPlayerIs2D)
        return 1800.f;

    return toggled ? 700.f : 270.f;
}

inline float fpHook() {
    return TwistsConfig::isSmallMarioEnabled() ? 3000.0f * scale : 3000.0f;
}

inline float fpScaleHook() {
    return TwistsConfig::isSmallMarioEnabled() ? 2.94f : 1.0f;
}

inline void capVelScaleHook(al::LiveActor* hackCap, sead::Vector3f const& addition) {
    if (!TwistsConfig::isSmallMarioEnabled()) {
        al::setVelocity(hackCap, addition);
        return;
    }
    sead::Vector3f newVelocity(addition.x * 0.45f, addition.y * 0.45f, addition.z * 0.45f);
    al::setVelocity(hackCap, newVelocity);
}

inline const char* offsetOverideHook(al::ByamlIter const& iter, char const* key) {
    if (!TwistsConfig::isSmallMarioEnabled())
        return nullptr;
    return "Y0.5m";
}

namespace smallMario {

static bool isHooksCreated = false;
static bool effectHookInstalled = false;

// ---------------------------------------------------------------------------
// RO value patches (physics constants, collider sizes)
// ---------------------------------------------------------------------------
struct RoPatch {
    u32 offset;
    u32 patchValue;
    u32 originalValue;
};

static RoPatch roPatches[] = {
    {0x435E88, 0x1E261008, 0},  // fmov s8, #16.0  - body collider radius
    {0x435E80, 0x1E267000, 0},  // fmov s0, #19.0  - body sphere shape
    {0x435ED8, 0x1E27D00A, 0},  // fmov s10, #30.0 - height
    {0x41B7F4, 0x1E249000, 0},  // fmov s0, #10.0  - dither anim sphere
    {0x3FF3F4, 0x1E2703F1, 0},  // fmov s17, #51.0 - cap throw height
    {0x48DFC8, 0x52800000, 0},  // mov w0, #0
    {0x4018D4, 0x52800041, 0},  // mov w1, #2
    {0x4464BC, 0x1E27D008, 0},  // fmov s8, #30.0
    {0x4406A0, 0x52800000, 0},  // tryEmitRollingEffect: mov w0, #0
    {0x4406A4, 0xD65F03C0, 0},  // tryEmitRollingEffect: ret
};

static constexpr int roPatchCount = hk::util::arraySize(roPatches);

// ---------------------------------------------------------------------------
// NOP patches
// ---------------------------------------------------------------------------
static constexpr u32 nopOffsets[] = {
    0x470908,  // hipdropland
    0x47BCB8,  // restartRolling
    0x47B12C,  // rollingBoostStart
    0x47AE80,  // rollingControl
    0x44038C,  // tryStartRunEffectRun
    0x4402AC,  // tryStartRunEffectRunStart
    0x44046C,  // tryStartRunEffectDash
    0x44054C,  // tryStartRunEffectDashFast
    0x44062C,  // tryStartRunEffectDashWaterSurface
};

static constexpr int nopCount = hk::util::arraySize(nopOffsets);
static u32 nopOriginals[nopCount];
static u32 followDistOriginal = 0;

static void initHooks() {
    if (isHooksCreated)
        return;

    uintptr_t base = hk::ro::getMainModule()->range().start();

    // Save RO patch originals
    for (int i = 0; i < roPatchCount; i++)
        roPatches[i].originalValue = *reinterpret_cast<u32*>(base + roPatches[i].offset);

    // Save NOP originals
    for (int i = 0; i < nopCount; i++)
        nopOriginals[i] = *reinterpret_cast<u32*>(base + nopOffsets[i]);

    // Save followDist original
    followDistOriginal = *reinterpret_cast<u32*>(base + 0xC8B9C);

    // Branch-link hooks
    hk::hook::writeBranchLinkAtMainOffset(0x4464F4, sensorHook);
    hk::hook::writeBranchLinkAtMainOffset(0x446538, sensorHook);
    hk::hook::writeBranchLinkAtMainOffset(0x44657C, sensorHook);
    hk::hook::writeBranchLinkAtMainOffset(0x4465C0, sensorHook);
    hk::hook::writeBranchLinkAtMainOffset(0x446604, sensorHook);
    hk::hook::writeBranchLinkAtMainOffset(0x446648, sensorHook);
    hk::hook::writeBranchLinkAtMainOffset(0x446674, sensorHook);
    hk::hook::writeBranchLinkAtMainOffset(0x4466BC, sensorHook);
    hk::hook::writeBranchLinkAtMainOffset(0x4466E4, sensorHook);
    hk::hook::writeBranchLinkAtMainOffset(0x44670C, sensorHook);
    hk::hook::writeBranchLinkAtMainOffset(0x3FFC24, capVelScaleHook);
    hk::hook::writeBranchLinkAtMainOffset(0x4466A8, fpHook);
    hk::hook::writeBranchLinkAtMainOffset(0x4018C4, fpScaleHook);
    hk::hook::writeBranchLinkAtMainOffset(0xA4BC70, offsetOverideHook);

    // effectHook
    effectHook.installAtSym<"_ZN2al16ActionEffectCtrl11startActionEPKc">();
    effectHookInstalled = true;

    isHooksCreated = true;
}

static void installHooks() {
    auto* mainModule = hk::ro::getMainModule();

    // Apply RO patches
    for (int i = 0; i < roPatchCount; i++)
        mainModule->writeRo(roPatches[i].offset, roPatches[i].patchValue);

    // Apply NOP patches
    for (int i = 0; i < nopCount; i++)
        mainModule->writeRo(nopOffsets[i], 0xD503201F);

    // Install followDist branch
    hk::hook::writeBranchAtMainOffset(0xC8B9C, followDistHook);
}

static void uninstallHooks() {
    auto* mainModule = hk::ro::getMainModule();

    // Restore RO patches
    for (int i = 0; i < roPatchCount; i++)
        mainModule->writeRo(roPatches[i].offset, roPatches[i].originalValue);

    // Restore NOP patches
    for (int i = 0; i < nopCount; i++)
        mainModule->writeRo(nopOffsets[i], nopOriginals[i]);

    // Restore followDist original instruction
    mainModule->writeRo(0xC8B9C, followDistOriginal);
}

}  // namespace smallMario