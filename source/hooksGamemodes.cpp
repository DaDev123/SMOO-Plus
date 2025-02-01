#include "al/util.hpp"
#include "al/util/NerveUtil.h"
#include "al/util/SensorUtil.h"
#include "game/GameData/GameDataFile.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"

#include "rs/util/InputUtil.h"
#include "rs/util/SensorUtil.h"
#include "server/Client.hpp"
#include "server/freeze/FreezeTagMode.hpp"
#include "server/hotpotato/HotPotatoMode.hpp"
#include "server/gamemode/GameModeManager.hpp"

#include "al/nerve/Nerve.h"
#include "rs/util.hpp"

bool newDeathArea(al::LiveActor const* player)
{
    // If player isn't actively playing Freeze Tag or Hot Potato, perform normal functionality
    if (!GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG) &&
        !GameModeManager::instance()->isModeAndActive(GameMode::HOTPOTATO))
        return al::isInDeathArea(player);

    // If player is in a death area but in Freeze Tag or Hot Potato mode, start a recovery event
    if (al::isInAreaObj(player, "DeathArea")) {
        FreezeTagMode* mode = GameModeManager::instance()->getMode<FreezeTagMode>();
        if (!mode->isEndgameActive())
            mode->tryStartRecoveryEvent(false);
    }
    // If player is in a death area but in Freeze Tag or Hot Potato mode, start a recovery event
    if (al::isInAreaObj(player, "DeathArea")) {
        HotPotatoMode* mode = GameModeManager::instance()->getMode<HotPotatoMode>();
        if (!mode->isEndgameActive())
            mode->tryStartRecoveryEvent(false);
    }

    return false;
}

void customPlayerHitPointDamage(PlayerHitPointData *thisPtr)
{
    // Disable damage in Freeze Tag or Hot Potato mode
    if (GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG) ||
        GameModeManager::instance()->isModeAndActive(GameMode::HOTPOTATO))
        return;
    
    int nextHit = thisPtr->mCurrentHit - 1;
    if (nextHit <= 0)
        nextHit = 0;
    
    thisPtr->mCurrentHit = nextHit;

    if (!thisPtr->mIsForceNormalHealth)
        if (nextHit <= (thisPtr->mIsKidsMode ? 6 : 3))
            thisPtr->mIsHaveMaxUpItem = false;
}

bool forceKidsMode(GameDataFile* thisPtr)
{
    // Enable Kids Mode in Freeze Tag or Hot Potato
    if (GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG) ||
        GameModeManager::instance()->isModeAndActive(GameMode::HOTPOTATO))
        return true;
    
    return thisPtr->mIsKidsMode;
}

bool customMoonHitboxDisable(al::IUseNerve* nrvUse, al::Nerve* nrv)
{
    // Disable moon hitbox in Freeze Tag or Hot Potato
    if (GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG) ||
        GameModeManager::instance()->isModeAndActive(GameMode::HOTPOTATO))
        return true;

    return al::isNerve(nrvUse, nrv);
}
