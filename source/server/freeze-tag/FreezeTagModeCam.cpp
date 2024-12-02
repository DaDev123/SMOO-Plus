#include "server/freeze-tag/FreezeTagMode.hpp"

#include "cameras/CameraPoserActorSpectate.h"

void FreezeTagMode::createCustomCameraTicket(al::CameraDirector* director) {
    mTicket = director->createCameraFromFactory("CameraPoserActorSpectate", nullptr, 0, 5, sead::Matrix34f::ident);
}

void FreezeTagMode::updateSpectateCam(PlayerActorBase* playerBase) {
    // If the specate camera ticket is active, get the camera poser
    al::CameraPoser*    curPoser = nullptr;
    al::CameraDirector* director = mCurScene->getCameraDirector();

    if (director) {
        al::CameraPoseUpdater* updater = director->getPoseUpdater(0);
        if (updater && updater->mTicket) {
            curPoser = updater->mTicket->mPoser;
        }
    }

    // Verify 100% that this poser is the actor spectator
    if (curPoser && al::isEqualString(curPoser->getName(), "CameraPoserActorSpectate")) {
        cc::CameraPoserActorSpectate* spectatePoser = (cc::CameraPoserActorSpectate*)curPoser;
        spectatePoser->setPlayer(playerBase);

        // Increase or decrease spectate index, followed by clamping it
        int indexDirection = 0;
        if (!isWipeout() && al::isPadTriggerRight(-1)) { indexDirection =  1; } // Move index right
        if (!isWipeout() && al::isPadTriggerLeft(-1))  { indexDirection = -1; } // Move index left

        // Force index towards ourself (-1) during endgame (wipeout) if we aren't already spectating ourselves
        if (isWipeout() && mSpectateIndex != -1) { indexDirection = -1; } // Move index left

        s32 size = mInfo->mRunnerPlayers.size();

        // Force index to decrease if our current index got out of bounds
        if (size <= mSpectateIndex) {
            mSpectateIndex = size;
            indexDirection = -1; // Move index left
        }

        PuppetInfo* other = (
            0 <= mSpectateIndex && mSpectateIndex < size
            ? mInfo->mRunnerPlayers.at(mSpectateIndex)
            : nullptr
        );

        // Force index to decrease if our current target is not ourself and changes to another stage
        if (indexDirection == 0 && other && (!other->isInSameStage || !other->isConnected)) {
            indexDirection = -1; // Move index left
        }

        if (indexDirection != 0) {
            // Loop over indicies until we find a runner in the same stage as the player or the player itself (-1 => spectate our own player)
            do {
                mSpectateIndex += indexDirection;

                // Circular loop the index around
                if (mSpectateIndex < -1) {
                    mSpectateIndex = size - 1;
                } else if (size <= mSpectateIndex) {
                    mSpectateIndex = -1;
                }

                other = (0 <= mSpectateIndex ? mInfo->mRunnerPlayers.at(mSpectateIndex) : nullptr);
            } while (other && (!other->isInSameStage || !other->isConnected));
        }

        // If no index and no size change is happening, end here (a size change could still be a player change at the same index)
        if (mPrevSpectateIndex == mSpectateIndex && mPrevSpectateCount == size) {
            return;
        }

        // Apply index to target actor and HUD
        if (other) {
            spectatePoser->setTargetActor(&other->playerPos);
            mModeLayout->setSpectateString(other->puppetName);
        } else {
            spectatePoser->setTargetActor(al::getTransPtr(playerBase));
            mModeLayout->setSpectateString("Spectate");
        }

        mPrevSpectateIndex = mSpectateIndex;
        mPrevSpectateCount = size;
    }
}
