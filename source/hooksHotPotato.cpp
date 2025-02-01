#include "al/util.hpp"
#include "al/util/NerveUtil.h"
#include "al/util/SensorUtil.h"
#include "game/GameData/GameDataFile.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"

#include "rs/util/InputUtil.h"
#include "rs/util/SensorUtil.h"
#include "server/Client.hpp"
#include "server/hotpotato/HotPotatoMode.hpp"
#include "server/gamemode/GameModeManager.hpp"

#include "al/nerve/Nerve.h"
#include "rs/util.hpp"

bool hotDeathArea(al::LiveActor const* player)
{
    // If player isn't actively playing freeze tag, perform normal functionality
    if (!GameModeManager::instance()->isModeAndActive(GameMode::HOTPOTATO))
        return al::isInDeathArea(player);

    // If player is in a death area but in Freeze Tag mode, start a recovery event
    if (al::isInAreaObj(player, "DeathArea")) {
        HotPotatoMode* mode = GameModeManager::instance()->getMode<HotPotatoMode>();
        if (!mode->isEndgameActive())
            mode->tryStartRecoveryEvent(false);
    }

    return false;
}

void hotPlayerHitPointDamage(PlayerHitPointData *thisPtr)
{
    if(GameModeManager::instance()->isModeAndActive(GameMode::HOTPOTATO))
        return;
    
    int nextHit = 0;
    int maxUpVal = 0;

    nextHit = thisPtr->mCurrentHit - 1;
    if (nextHit <= 0)
        nextHit = 0;
    
    thisPtr->mCurrentHit = nextHit;

    if (!thisPtr->mIsForceNormalHealth )
        if (nextHit <= (thisPtr->mIsKidsMode ? 6 : 3))
            thisPtr->mIsHaveMaxUpItem = false;
}

bool hotKidsMode(GameDataFile* thisPtr)
{
    if(GameModeManager::instance()->isModeAndActive(GameMode::HOTPOTATO))
        return true;
    
    return thisPtr->mIsKidsMode;
}

bool hotMoonHitboxDisable(al::IUseNerve* nrvUse, al::Nerve* nrv)
{
    if(GameModeManager::instance()->isModeAndActive(GameMode::HOTPOTATO))
        return true;

    return al::isNerve(nrvUse, nrv);
}