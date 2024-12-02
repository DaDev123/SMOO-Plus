#include "al/util.hpp"

#include "game/GameData/GameDataFile.h"

#include "server/freeze-tag/FreezeTagMode.hpp"
#include "server/gamemode/GameModeManager.hpp"


bool freezeIsCheckpointWarpAllowed() {
    return !GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG);
}

bool freezeDeathArea(al::LiveActor const* player) {
    // If player isn't actively playing freeze tag, perform normal functionality
    if (!GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG)) {
        return al::isInDeathArea(player);
    }

    // If player is in a death area but in Freeze Tag mode, start a recovery event
    if (al::isInAreaObj(player, "DeathArea")) {
        FreezeTagMode* mode = GameModeManager::instance()->getMode<FreezeTagMode>();
        if (!mode->isWipeout()) {
            mode->tryStartRecoveryEvent(false);
        }
    }

    return false;
}

void freezePlayerHitPointDamage(PlayerHitPointData* thisPtr) {
    if (GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG)) {
        return;
    }

    int nextHit  = 0;
    int maxUpVal = 0;

    nextHit = thisPtr->mCurrentHit - 1;
    if (nextHit <= 0) {
        nextHit = 0;
    }

    thisPtr->mCurrentHit = nextHit;

    if (!thisPtr->mIsForceNormalHealth ) {
        if (nextHit <= (thisPtr->mIsKidsMode ? 6 : 3)) {
            thisPtr->mIsHaveMaxUpItem = false;
        }
    }
}

bool freezeKidsMode(GameDataFile* thisPtr) {
    if (GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG)) {
        return true;
    }

    return thisPtr->mIsKidsMode;
}

bool freezeMoonHitboxDisable(al::IUseNerve* nrvUse, al::Nerve* nrv) {
    if (GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG)) {
        return true;
    }

    return al::isNerve(nrvUse, nrv);
}
