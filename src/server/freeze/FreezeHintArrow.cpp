#include "server/freeze/FreezeHintArrow.h"

#include "al/Library/LiveActor/ActorModelFunction.h"

#include "server/gamemode/GameModeManager.hpp"

FreezeHintArrow::FreezeHintArrow(const char* name) : GameModeHintArrow(name) {}

void FreezeHintArrow::initAfterPlacement(void) {
    GameModeHintArrow::initAfterPlacement();

    if (GameModeManager::instance()->isMode(GameMode::FREEZETAG))
        mInfo = GameModeManager::instance()->getInfo<FreezeTagInfo>();
}

void FreezeHintArrow::setupMaterials() {
    al::showMaterial(this, "BodyRedMT00");
    al::hideMaterial(this, "BodyYellowMT00");
    al::hideMaterial(this, "BodyBlueMT00");
}

bool FreezeHintArrow::shouldBeVisible() {
    if (!mInfo)
        return false;

    bool isInFreezeMode = GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG);
    return isInFreezeMode && !mInfo->mIsPlayerRunner && mInfo->mIsRound;
}