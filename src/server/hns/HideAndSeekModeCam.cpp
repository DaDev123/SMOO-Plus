#include "al/Library/Camera/CameraDirector.h"
#include "al/Library/Camera/CameraPoserUpdater.h"
#include "al/Library/Controller/InputFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"

#include "cameras/CameraPoserActorSpectate.h"
#include "server/Client.hpp"
#include "server/hns/HideAndSeekMode.hpp"

void HideAndSeekMode::updateSpectateCam(PlayerActorBase* playerBase) {
    // Add null checks for critical objects
    if (!mTicket) {
        Logger::log("ERROR: mTicket is null!\n");
        return;
    }

    if (!mCurScene) {
        Logger::log("ERROR: mCurScene is null!\n");
        return;
    }

    if (!playerBase) {
        Logger::log("ERROR: playerBase is null!\n");
        return;
    }

    if (!mTicket->mIsActiveCamera) {
        Logger::log("DEBUG: Camera ticket not active\n");
        return;
    }

    // Get camera director with null checks
    al::CameraDirector* director = mCurScene->getCameraDirector();
    if (!director) {
        Logger::log("ERROR: No camera director found\n");
        return;
    }

    // Get pose updater with bounds checking
    al::CameraPoseUpdater* updater = director->getPoseUpdater(0);
    if (!updater) {
        Logger::log("ERROR: No pose updater found\n");
        return;
    }

    if (!updater->mTicket) {
        Logger::log("ERROR: Updater ticket is null\n");
        return;
    }

    al::CameraPoser* curPoser = updater->mTicket->mPoser;
    if (!curPoser) {
        Logger::log("ERROR: Camera poser is null\n");
        return;
    }

    Logger::log("DEBUG: Found camera poser: %s\n", curPoser->getName());

    // Verify this is the spectate poser
    if (!al::isEqualString(curPoser->getName(), "CameraPoserActorSpectate")) {
        Logger::log("ERROR: Wrong camera poser type: %s\n", curPoser->getName());
        return;
    }

    cc::CameraPoserActorSpectate* spectatePoser = (cc::CameraPoserActorSpectate*)curPoser;

    // Set player on spectate poser
    spectatePoser->setPlayer(playerBase);

    // Count valid players with bounds checking
    int validPlayerCount = 0;
    int totalPuppets = mPuppetHolder ? mPuppetHolder->getSize() : 0;

    if (totalPuppets <= 0) {
        Logger::log("DEBUG: No puppets available\n");
        return;
    }

    Logger::log("DEBUG: Total puppets in holder: %d\n", totalPuppets);

    // Count valid players with proper bounds checking
    for (int i = 0; i < totalPuppets; i++) {
        PuppetInfo* curInfo = Client::getPuppetInfo(i);
        if (curInfo && curInfo->isConnected && curInfo->isInSameStage) {
            validPlayerCount++;
            Logger::log("DEBUG: Valid puppet %d - name: %s\n", i, curInfo->puppetName);
        }
    }

    Logger::log("DEBUG: Valid player count: %d, current spectate index: %d\n", validPlayerCount, mSpectateIndex);

    // Handle input for changing spectate target
    int indexDirection = 0;
    if (al::isPadTriggerRight(-1)) {
        indexDirection = 1;
        Logger::log("DEBUG: Right trigger pressed!\n");
    }
    if (al::isPadTriggerLeft(-1)) {
        indexDirection = -1;
        Logger::log("DEBUG: Left trigger pressed!\n");
    }

    // Bounds checking for spectate index
    if (mSpectateIndex >= validPlayerCount) {
        indexDirection = -1;
    }

    // Validate current target is still valid
    if (mSpectateIndex != -1 && indexDirection == 0) {
        int currentValidIndex = 0;
        bool foundValidTarget = false;

        for (int i = 0; i < totalPuppets && currentValidIndex <= mSpectateIndex; i++) {
            PuppetInfo* curInfo = Client::getPuppetInfo(i);
            if (curInfo && curInfo->isConnected && curInfo->isInSameStage) {
                if (currentValidIndex == mSpectateIndex) {
                    foundValidTarget = true;
                    break;
                }
                currentValidIndex++;
            }
        }

        if (!foundValidTarget) {
            indexDirection = -1;
        }
    }

    // Find next valid spectate target
    if (indexDirection != 0) {
        int attempts = 0;
        const int maxAttempts = validPlayerCount + 2;  // Prevent infinite loops

        while (attempts < maxAttempts) {
            mSpectateIndex += indexDirection;

            // Clamp index (-1 = self spectate)
            if (mSpectateIndex < -1)
                mSpectateIndex = validPlayerCount - 1;
            if (mSpectateIndex >= validPlayerCount)
                mSpectateIndex = -1;

            // -1 is always valid (self spectate)
            if (mSpectateIndex == -1) {
                break;
            } else {
                // Check if this index corresponds to a valid player
                int currentValidIndex = 0;
                bool foundValid = false;
                for (int i = 0; i < totalPuppets; i++) {
                    PuppetInfo* curInfo = Client::getPuppetInfo(i);
                    if (curInfo && curInfo->isConnected && curInfo->isInSameStage) {
                        if (currentValidIndex == mSpectateIndex) {
                            foundValid = true;
                            break;
                        }
                        currentValidIndex++;
                    }
                }
                if (foundValid)
                    break;
            }
            attempts++;
        }

        // Safety fallback
        if (attempts >= maxAttempts) {
            mSpectateIndex = -1;
            Logger::log("WARNING: Spectate index search exceeded max attempts, falling back to self\n");
        }
    }

    // If no change, return early
    if (mPrevSpectateIndex == mSpectateIndex)
        return;

    // Apply spectate target with proper error handling
    if (mSpectateIndex == -1) {
        // Spectate self - use player's transform pointer
        sead::Vector3f* playerTransPtr = al::getTransPtr(playerBase);
        if (playerTransPtr) {
            spectatePoser->setTargetActor(playerTransPtr);
            Logger::log("Now spectating self\n");
        } else {
            Logger::log("ERROR: Could not get player transform pointer\n");
            return;
        }
    } else {
        // Find the target puppet info
        int currentValidIndex = 0;
        PuppetInfo* targetInfo = nullptr;

        for (int i = 0; i < totalPuppets; i++) {
            PuppetInfo* curInfo = Client::getPuppetInfo(i);
            if (curInfo && curInfo->isConnected && curInfo->isInSameStage) {
                if (currentValidIndex == mSpectateIndex) {
                    targetInfo = curInfo;
                    break;
                }
                currentValidIndex++;
            }
        }

        if (targetInfo) {
            // CRITICAL FIX: Use a stable pointer to the position
            // The issue might be that playerPos is being modified while we're using it
            // Create a local copy or ensure the pointer remains valid
            spectatePoser->setTargetActor(&targetInfo->playerPos);

            Logger::log("Now spectating player: %s at index %d\n", strcmp(targetInfo->puppetName, "") != 0 ? targetInfo->puppetName : "Unknown",
                        mSpectateIndex);
        } else {
            // Fallback to self if target not found
            Logger::log("WARNING: Could not find spectate target, falling back to self\n");
            mSpectateIndex = -1;
            sead::Vector3f* playerTransPtr = al::getTransPtr(playerBase);
            if (playerTransPtr) {
                spectatePoser->setTargetActor(playerTransPtr);
            } else {
                Logger::log("ERROR: Could not get player transform pointer for fallback\n");
                return;
            }
        }
    }

    mPrevSpectateIndex = mSpectateIndex;
}