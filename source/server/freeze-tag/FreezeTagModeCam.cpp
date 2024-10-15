#include "server/freeze-tag/FreezeTagMode.hpp"
#include "server/freeze-tag/FreezeTagIcon.h"
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
        if (!mIsEndgameActive && al::isPadTriggerRight(-1)) { indexDirection =  1; } // Move index right
        if (!mIsEndgameActive && al::isPadTriggerLeft(-1))  { indexDirection = -1; } // Move index left

        // Force index towards ourself (-1) during endgame if we aren't already spectating ourselves
        // Force index to decrease if our current index is higher than the current runner count
        if ((mIsEndgameActive && mSpectateIndex != -1) || mInfo->mRunnerPlayers.size() <= mSpectateIndex) {
            indexDirection = -1; // Move index left
        }

        // Force index to decrease if our current target is not ourself and changes to another stage
        if (indexDirection == 0 && mSpectateIndex != -1 && !mInfo->mRunnerPlayers.at(mSpectateIndex)->isInSameStage) {
            indexDirection = -1; // Move index left
            // RCL TODO: what happens if they disconnect but aren't the last one in the list??
        }

        // no direction, end here
        if (indexDirection == 0) {
            return;
        }

        // Loop over indicies until we find a runner in the same stage as the player or the player itself (-1 => spectate our own player)
        do {
            mSpectateIndex += indexDirection;

            // Circular loop the index around
            if (mSpectateIndex < -1) {
                mSpectateIndex = mInfo->mRunnerPlayers.size() - 1;
            } else if (mInfo->mRunnerPlayers.size() <= mSpectateIndex) {
                mSpectateIndex = -1;
            }
        } while (mSpectateIndex != -1 && !mInfo->mRunnerPlayers.at(mSpectateIndex)->isInSameStage);

        // If no index change is happening, end here
        if (mPrevSpectateIndex == mSpectateIndex) {
            return;
        }

        // Apply index to target actor and HUD
        if (mSpectateIndex == -1) {
            spectatePoser->setTargetActor(al::getTransPtr(playerBase));
            mModeLayout->setSpectateString("Spectate");
        } else {
            PuppetInfo* other = mInfo->mRunnerPlayers.at(mSpectateIndex);
            spectatePoser->setTargetActor(&other->playerPos);
            mModeLayout->setSpectateString(other->puppetName);
        }

        mPrevSpectateIndex = mSpectateIndex;
    }
}
