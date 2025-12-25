
#pragma once
#include "hk/hook/Replace.h"
#include "hk/hook/Trampoline.h"

#include "al/Library/LiveActor/ActorAreaFunction.h"
#include "al/Library/Nerve/NerveUtil.h"

#include "game/System/GameDataFile.h"

#include "server/freeze/FreezeTagMode.hpp"
#include "server/gamemode/GameModeManager.hpp"

static HkReplace<bool, MapLayout*> isCheckpointWarpAllowedHook =
    hk::hook::replace([](MapLayout* map) -> bool { return !GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG); });

static HkTrampoline<bool, al::LiveActor*> freezeDeathAreaHook = hk::hook::trampoline([](al::LiveActor* actor) -> bool {
    bool isHakoniwa = al::isEqualString(typeid(*actor).name(), typeid(PlayerActorHakoniwa).name());
    if (!GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG) || !isHakoniwa) {
        return freezeDeathAreaHook.orig(actor);
    }
    if (al::isInAreaObj(actor, "DeathArea")) {
        FreezeTagMode* mode = GameModeManager::instance()->getMode<FreezeTagMode>();
        if (!mode->isEndgameActive()) {
            mode->tryStartRecoveryEvent(false);
        }
    }
    return false;
});

static HkTrampoline<void, PlayerHitPointData*> playerHitPointDamageHook = hk::hook::trampoline([](PlayerHitPointData* hitData) -> void {
    if (!GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG)) {
        playerHitPointDamageHook.orig(hitData);
    }
});

static HkTrampoline<bool, HackCap*> isEnableRescuePlayerHook = hk::hook::trampoline(
    [](HackCap* hackCap) -> bool { return GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG) ? true : isEnableRescuePlayerHook.orig(hackCap); });

static HkTrampoline<bool, Shine*, al::SensorMsg*, al::HitSensor*, al::HitSensor*> freezeMoonHitboxHook =
    hk::hook::trampoline([](Shine* shine, al::SensorMsg* msg, al::HitSensor* sourve, al::HitSensor* target) -> bool {
        if (GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG)) {
            return true;
        }
        return freezeMoonHitboxHook.orig(shine, msg, sourve, target);
    });