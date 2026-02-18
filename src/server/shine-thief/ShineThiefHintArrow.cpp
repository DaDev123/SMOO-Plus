#include "server/shine-thief/ShineThiefHintArrow.h"

#include "al/Library/LiveActor/ActorModelFunction.h"

#include "server/gamemode/GameModeManager.hpp"

ShineThiefHintArrow::ShineThiefHintArrow(const char* name) : GameModeHintArrow(name) {}

void ShineThiefHintArrow::initAfterPlacement(void) {
    GameModeHintArrow::initAfterPlacement();

    if (GameModeManager::instance()->isMode(GameMode::SHINETHIEF))
        mInfo = GameModeManager::instance()->getInfo<ShineThiefInfo>();
}

void ShineThiefHintArrow::setupMaterials() {
    bool showYellow = true;
    bool showBlue = false;
    bool showRed = false;

    if (mInfo && mInfo->mIsTeamMode) {
        showYellow = false;

        // Get the holder's team, not the local player's team
        ShineThiefTeam holderTeam = ShineThiefTeam::NONE;

        if (mInfo->mIsPlayerHolder) {
            holderTeam = mInfo->mPlayerTeam;
        } else if (mInfo->mHolderPlayers.size() > 0) {
            holderTeam = (ShineThiefTeam)mInfo->mHolderPlayers.at(0)->shineThiefTeam;
        }

        if (holderTeam == ShineThiefTeam::TEAM_1)
            showBlue = true;
        else if (holderTeam == ShineThiefTeam::TEAM_2)
            showRed = true;
        else
            showYellow = true;  // NONE fallback
    }

    if (showYellow)
        al::showMaterial(this, "BodyYellowMT00");
    else
        al::hideMaterial(this, "BodyYellowMT00");

    if (showBlue)
        al::showMaterial(this, "BodyBlueMT00");
    else
        al::hideMaterial(this, "BodyBlueMT00");

    if (showRed)
        al::showMaterial(this, "BodyRedMT00");
    else
        al::hideMaterial(this, "BodyRedMT00");
}

bool ShineThiefHintArrow::shouldBeVisible() {
    if (!mInfo)
        return false;

    bool isInShineThiefMode = GameModeManager::instance()->isModeAndActive(GameMode::SHINETHIEF);
    return isInShineThiefMode && !mInfo->mIsPlayerHolder && mInfo->mIsRound;
}