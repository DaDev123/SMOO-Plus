#include "al/util.hpp"

#include "game/GameData/GameDataFile.h"

#include "server/coinrunners/CoinRunnerMode.hpp"
#include "server/gamemode/GameModeManager.hpp"


bool coinIsCheckpointWarpAllowed() {
    return !GameModeManager::instance()->isModeAndActive(GameMode::COINRUNNER);
}

bool coinDeathArea(al::LiveActor const* player) {
    // If player isn't actively playing freeze tag, perform normal functionality
    if (!GameModeManager::instance()->isModeAndActive(GameMode::COINRUNNER)) {
        return al::isInDeathArea(player);
    }

    // If player is in a death area but in Freeze Tag mode, start a recovery event
    if (al::isInAreaObj(player, "DeathArea")) {
        CoinRunnerMode* mode = GameModeManager::instance()->getMode<CoinRunnerMode>();
        if (!mode->isWipeout()) {
            mode->tryStartRecoveryEvent(false);
        }
    }

    return false;
}

void coinPlayerHitPointDamage(PlayerHitPointData* thisPtr) {
    if (GameModeManager::instance()->isModeAndActive(GameMode::COINRUNNER)) {
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

bool coinKidsMode(GameDataFile* thisPtr) {
    if (GameModeManager::instance()->isModeAndActive(GameMode::COINRUNNER)) {
        return true;
    }

    return thisPtr->mIsKidsMode;
}

bool coinMoonHitboxDisable(al::IUseNerve* nrvUse, al::Nerve* nrv) {
    if (GameModeManager::instance()->isModeAndActive(GameMode::COINRUNNER)) {
        return true;
    }

    return al::isNerve(nrvUse, nrv);
}
