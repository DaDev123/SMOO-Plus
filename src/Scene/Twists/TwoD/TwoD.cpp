#include "Scene/Twists/TwoD/TwoD.hpp"

#include "hk/hook/Trampoline.h"
#include "hk/ro/RoUtil.h"
#include "hk/util/Math.h"

#include "al/Library/Controller/InputFunction.h"
#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/LiveActor/ActorSensorUtil.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "al/Library/Player/PlayerUtil.h"
#include "al/Library/Scene/SceneUtil.h"
#include "al/Project/HitSensor/HitSensor.h"

#include "game/Player/PlayerAnimator.h"
#include "game/Player/PlayerFunction.h"
#include "game/Player/PlayerHackKeeper.h"
#include "game/Util/ActorDimensionKeeper.h"
#include "game/Util/DemoUtil.h"
#include "game/Util/SensorMsgFunction.h"

#include "rs/util.hpp"
#include "Scene/Twists/TwoD/NrvPlayerActorHakoniwa.h"

bool TwoDTwist::sTwoDEnabled = false;
ActorDimensionKeeper* TwoDTwist::sDimensionKeeper = nullptr;
PlayerActorHakoniwa* TwoDTwist::sPlayer = nullptr;
int TwoDTwist::sSceneFrames = 0;

static HkTrampoline<void, ActorDimensionKeeper*> updateDimensionKeeperHook = hk::hook::trampoline([](ActorDimensionKeeper* keeper) -> void {
    updateDimensionKeeperHook.orig(keeper);

    if (!TwoDTwist::sTwoDEnabled)
        return;
    if (!TwoDTwist::sDimensionKeeper)
        return;
    if (keeper != TwoDTwist::sDimensionKeeper)
        return;

    keeper->mIs2D = TwoDTwist::sTwoDEnabled || keeper->mIsIn2DArea;
});

static HkTrampoline<u64, al::LiveActor*, int> isHoldActionHook = hk::hook::trampoline([](al::LiveActor* actor, int port) -> u64 {
    if (TwoDTwist::sTwoDEnabled && TwoDTwist::sSceneFrames < 120)
        return 0;
    return isHoldActionHook.orig(actor, port);
});

static HkTrampoline<bool, PlayerActorHakoniwa*> updateNerveStatePlayerHook = hk::hook::trampoline([](PlayerActorHakoniwa* player) -> bool {
    if (TwoDTwist::sTwoDEnabled && player == TwoDTwist::sPlayer && TwoDTwist::sSceneFrames < 120)
        return false;
    return updateNerveStatePlayerHook.orig(player);
});

static HkTrampoline<void, void*> setNerveOnGroundHook = hk::hook::trampoline([](void* self) -> void {
    if (TwoDTwist::sTwoDEnabled && TwoDTwist::sSceneFrames < 120)
        return;
    setNerveOnGroundHook.orig(self);
});

static HkTrampoline<void, void*> exeRun3DHakoniwaHook = hk::hook::trampoline([](void* self) -> void {
    if (TwoDTwist::sTwoDEnabled) {
        using Fn = void (*)(void*);
        static Fn exeRun2D = reinterpret_cast<Fn>(hk::ro::getMainModule()->range().start() + 0x4801b0);
        exeRun2D(self);
        return;
    }
    exeRun3DHakoniwaHook.orig(self);
});

static HkTrampoline<void, void*> exeRun3DHook = hk::hook::trampoline([](void* self) -> void {
    if (TwoDTwist::sTwoDEnabled) {
        using Fn = void (*)(void*);
        static Fn exeRun2D = reinterpret_cast<Fn>(hk::ro::getMainModule()->range().start() + 0x47e5bc);
        exeRun2D(self);
        return;
    }
    exeRun3DHook.orig(self);
});

static void sUpdateSensorForm(PlayerFormSensorCollisionArranger* form) {
    if (!TwoDTwist::sDimensionKeeper || !TwoDTwist::sDimensionKeeper->mIsIn2DArea)
        form->setFormModel3D();
    else
        form->setFormModel2D();
}

static bool sIsUpperBodyAttachmentPatch(PlayerAnimator* animator) {
    return false;
}

static bool sPlayerItemGetMsg(al::HitSensor* hitTarget, al::HitSensor* player) {
    rs::sendMsgPlayerItemGetAll2D(hitTarget, player);
    rs::sendMsgPlayerHipDropHipDropSwitch(hitTarget, player);
    al::sendMsgPlayerHipDrop(hitTarget, player, nullptr);
    al::sendMsgPlayerKick(hitTarget, player);
    al::sendMsgPlayerTouch(hitTarget, player);
    return rs::sendMsgPlayerItemGetAll(hitTarget, player);
}

static float sDashMaxSpeed() {
    return (TwoDTwist::sDimensionKeeper && TwoDTwist::sDimensionKeeper->mIs2D) ? 25.f : 17.f;
}
static float sJumpPowerMin() {
    return (TwoDTwist::sDimensionKeeper && TwoDTwist::sDimensionKeeper->mIs2D) ? 18.5f : 17.f;
}
static float sJumpPowerMax() {
    return (TwoDTwist::sDimensionKeeper && TwoDTwist::sDimensionKeeper->mIs2D) ? 31.f : 19.5f;
}
static float sNormalMaxSpeed() {
    return (TwoDTwist::sDimensionKeeper && TwoDTwist::sDimensionKeeper->mIs2D) ? 12.f : 10.f;
}
static float sNormalMinSpeed() {
    return (TwoDTwist::sDimensionKeeper && TwoDTwist::sDimensionKeeper->mIs2D) ? 2.5f : 3.f;
}

struct RoPatch {
    u32 offset;
    u32 patchValue;
    u32 originalValue;
};

static RoPatch sRoPatches[] = {
    {0x25b36c, 0x52800018, 0},  // MOV W24, #0  — always allow pipe enters
    {0x427E00, 0xD503201F, 0},  // NOP          — always send 2D trample msg
    {0x4293e8, 0x52800008, 0},  // MOV X8, #0   — make all push msgs 3D
    {0x427e14, 0x14000001, 0},  // B 427e18     — jump to 3D trample msg
    {0x0888b8, 0x140003F3, 0},  // B 089884     — bowser hat fix
    {0x0898a0, 0x1000000F, 0},  // B 0898dc     — bowser hat fix
    {0x41d888, 0x14000003, 0},  // B 41d894     — skip vanilla is2D check, go to our BL
};
static constexpr int sRoPatchCount = hk::util::arraySize(sRoPatches);

static u32 sDamageCheck1Orig = 0;
static u32 sDamageCheck2Orig = 0;
static u32 sPhysicsOrig[5] = {};
static constexpr u32 sPhysicsOffsets[5] = {0x43cfdc, 0x43d44c, 0x43d454, 0x43cfd4, 0x43cfcc};

static u32 sSensorFormOrig = 0;
static u32 sUpperBodyOrig = 0;
static u32 sItemGetMsg1Orig = 0;
static u32 sItemGetMsg2Orig = 0;

static bool sInstalled = false;
static bool sBranchLinksInstalled = false;

static u32 makeBranch(uintptr_t from, uintptr_t to) {
    s32 delta = (s32)(to - from) / 4;
    return 0x14000000u | (u32)(delta & 0x3FFFFFF);
}

static u32 makeBranchLink(uintptr_t from, uintptr_t to) {
    s32 delta = (s32)(to - from) / 4;
    return 0x94000000u | (u32)(delta & 0x3FFFFFF);
}

void TwoDTwist::initHooks() {
    if (sBranchLinksInstalled)
        return;

    auto* mod = hk::ro::getMainModule();
    if (!mod)
        return;

    uintptr_t base = mod->range().start();

    for (int i = 0; i < sRoPatchCount; i++)
        sRoPatches[i].originalValue = *reinterpret_cast<u32*>(base + sRoPatches[i].offset);
    for (int i = 0; i < 5; i++)
        sPhysicsOrig[i] = *reinterpret_cast<u32*>(base + sPhysicsOffsets[i]);
    sDamageCheck1Orig = *reinterpret_cast<u32*>(base + 0x429134);
    sDamageCheck2Orig = *reinterpret_cast<u32*>(base + 0x429154);
    sSensorFormOrig = *reinterpret_cast<u32*>(base + 0x41d894);
    sUpperBodyOrig = *reinterpret_cast<u32*>(base + 0x471378);
    sItemGetMsg1Orig = *reinterpret_cast<u32*>(base + 0x427ba0);
    sItemGetMsg2Orig = *reinterpret_cast<u32*>(base + 0x427b8c);

    isHoldActionHook.installAtSym<"_ZN19PlayerInputFunction12isHoldActionEPKN2al9LiveActorEi">();
    updateDimensionKeeperHook.installAtSym<"_ZN20ActorDimensionKeeper6updateEv">();
    updateNerveStatePlayerHook.installAtSym<"_ZN2al16updateNerveStateEPNS_9IUseNerveE">();
    setNerveOnGroundHook.installAtSym<"_ZN19PlayerActorHakoniwa16setNerveOnGroundEv">();
    exeRun3DHakoniwaHook.installAtSym<"_ZN26PlayerStateRunHakoniwa2D3D8exeRun3DEv">();
    exeRun3DHook.installAtSym<"_ZN18PlayerStateRun2D3D8exeRun3DEv">();

    sBranchLinksInstalled = true;
}

static void installPatches() {
    if (sInstalled)
        return;

    auto* mod = hk::ro::getMainModule();
    if (!mod)
        return;

    uintptr_t base = mod->range().start();

    for (int i = 0; i < sRoPatchCount; i++)
        mod->writeRo(sRoPatches[i].offset, sRoPatches[i].patchValue);

    hk::hook::writeBranchLinkAtMainOffset(0x41d894, sUpdateSensorForm);
    hk::hook::writeBranchLinkAtMainOffset(0x471378, sIsUpperBodyAttachmentPatch);
    hk::hook::writeBranchLinkAtMainOffset(0x427ba0, sPlayerItemGetMsg);
    hk::hook::writeBranchLinkAtMainOffset(0x427b8c, sPlayerItemGetMsg);

    void* physFuncs[5] = {
        (void*)sDashMaxSpeed, (void*)sJumpPowerMin, (void*)sJumpPowerMax, (void*)sNormalMaxSpeed, (void*)sNormalMinSpeed,
    };
    for (int i = 0; i < 5; i++)
        mod->writeRo(sPhysicsOffsets[i], makeBranch(base + sPhysicsOffsets[i], (uintptr_t)physFuncs[i]));

    mod->writeRo(0x429134, makeBranchLink(base + 0x429134, base + 0x593244));
    mod->writeRo(0x429154, makeBranchLink(base + 0x429154, base + 0x593244));

    sInstalled = true;
}

static void uninstallPatches() {
    if (!sInstalled)
        return;

    auto* mod = hk::ro::getMainModule();
    if (!mod)
        return;

    for (int i = 0; i < sRoPatchCount; i++)
        mod->writeRo(sRoPatches[i].offset, sRoPatches[i].originalValue);
    for (int i = 0; i < 5; i++)
        mod->writeRo(sPhysicsOffsets[i], sPhysicsOrig[i]);

    mod->writeRo(0x429134, sDamageCheck1Orig);
    mod->writeRo(0x429154, sDamageCheck2Orig);
    mod->writeRo(0x41d894, sSensorFormOrig);
    mod->writeRo(0x471378, sUpperBodyOrig);
    mod->writeRo(0x427ba0, sItemGetMsg1Orig);
    mod->writeRo(0x427b8c, sItemGetMsg2Orig);

    sInstalled = false;
}

void TwoDTwist::toggleTwoD() {
    if (!sBranchLinksInstalled)
        return;

    sTwoDEnabled = !sTwoDEnabled;

    if (sTwoDEnabled) {
        installPatches();
    } else {
        uninstallPatches();
        if (sDimensionKeeper)
            sDimensionKeeper->mIs2D = false;
    }

    if (sPlayer) {
        sPlayer->startDemoPuppetable();
        sPlayer->mAnimator->startAnim("Wait");
        sPlayer->endDemoPuppetable();
    }
}

void TwoDTwist::onStageInit(StageScene* scene, PlayerActorHakoniwa* p1) {
    sSceneFrames = 0;
    sDimensionKeeper = nullptr;
    sPlayer = nullptr;

    if (!p1)
        return;

    sPlayer = p1;
    sDimensionKeeper = p1->mDimensionKeeper;
}

void TwoDTwist::onStageDeath() {
    sSceneFrames = 0;
    sDimensionKeeper = nullptr;
    sPlayer = nullptr;
}

void TwoDTwist::update(StageScene* scene, PlayerActorHakoniwa* p1, bool isFirstStep) {
    if (!p1)
        return;

    if (isFirstStep)
        onStageInit(scene, p1);

    if (!sTwoDEnabled || !sDimensionKeeper)
        return;

    bool isPause = scene->isPause();
    bool isDemo = rs::isActiveDemo(p1);
    bool isDead = PlayerFunction::isPlayerDeadStatus(p1);
    bool isCapture = p1->mHackKeeper->mHackActor != nullptr;
    bool isBind = al::isNerve(p1, reinterpret_cast<al::Nerve*>(nrvPlayerActorHakoniwaBind));

    bool isInterrupted = isPause || isDemo || isDead || isCapture || isBind;

    if (!isInterrupted)
        sSceneFrames++;

    sDimensionKeeper->mIs2D = sTwoDEnabled || sDimensionKeeper->mIsIn2DArea;

    if (sSceneFrames < 120 && (isBind || isCapture || isDemo))
        sDimensionKeeper->mIs2D = false;
}

bool TwoDTwist::isIn2DMode() {
    return sDimensionKeeper && sDimensionKeeper->mIs2D;
}