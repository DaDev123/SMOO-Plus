#include "server/freeze-tag/FreezeTagMode.hpp"

#include "rs/util.hpp"

bool FreezeTagMode::areAllOtherRunnersFrozen(PuppetInfo* player) {
    if (runners() < 1) {
        return false; // Verify there is at least one runners (including yourself), otherwise disable this functionality
        /**
         * old legacy clients used a minimum size of 2 runners here.
         *
         * this lead to a bug that if there was only one runner a round could never be won by chasers.
         * changing this breaks compatibility with legacy clients about when a round ends or not.
         *
         * therefore legacy clients will reveice an extra ROUNDCANCEL packet that only affects them.
         * ending a round in this way will prevent legacy clients from getting score points for a WIPEOUT
         * in this situation (only one runner), though that's better than it was before (round continues even
         * though the only runner is frozen already).
         */
    }

    if (isPlayerRunner() && isPlayerUnfrozen()) {
        return false; // If you are a runner but aren't frozen then skip
    }

    for (int i = 0; i < mInfo->mRunnerPlayers.size(); i++) {
        PuppetInfo* other = mInfo->mRunnerPlayers.at(i);
        if (other == player) {
            continue; // If the puppet getting updated is the one currently being checked, skip this one
        }

        if (other->ftIsUnfrozen()) {
            return false; // Found a non-frozen player on the runner team, cancel
        }
    }

    return true; // All runners are frozen!
}

PlayerActorHakoniwa* FreezeTagMode::getPlayerActorHakoniwa() {
    PlayerActorBase* playerBase = rs::getPlayerActor(mCurScene);
    bool isYukimaru = !playerBase->getPlayerInfo();

    if (isYukimaru) {
        return nullptr;
    }

    return (PlayerActorHakoniwa*)playerBase;
}
