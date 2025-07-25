#include "server/hns/HideAndSeekMode.hpp"
#include "cameras/CameraPoserActorSpectate.h"
#include "server/Client.hpp"

void HideAndSeekMode::updateSpectateCam(PlayerActorBase* playerBase)
{
    // Debug: Check if we even have a ticket
    if (!mTicket) {
        Logger::log("ERROR: mTicket is null!\n");
        return;
    }
    
    if (!mTicket->mIsActive) {
        Logger::log("DEBUG: Camera ticket not active\n");
        return;
    }

    //If the spectate camera ticket is active, get the camera poser
    al::CameraPoser* curPoser = nullptr;
    al::CameraDirector* director = mCurScene->getCameraDirector();

    if (director) {
        al::CameraPoseUpdater* updater = director->getPoseUpdater(0);
        if (updater && updater->mTicket) {
            curPoser = updater->mTicket->mPoser;
            Logger::log("DEBUG: Found camera poser: %s\n", curPoser ? curPoser->getName() : "null");
        } else {
            Logger::log("ERROR: No updater or updater ticket found\n");
            return;
        }
    } else {
        Logger::log("ERROR: No camera director found\n");
        return;
    }
    
    //Verify 100% that this poser is the actor spectator
    if (curPoser && al::isEqualString(curPoser->getName(), "CameraPoserActorSpectate")) {
        cc::CameraPoserActorSpectate* spectatePoser = (cc::CameraPoserActorSpectate*)curPoser;
        spectatePoser->setPlayer(playerBase);

        // Count valid players and get total puppet count
        int validPlayerCount = 0;
        int totalPuppets = mPuppetHolder->getSize();
        
        Logger::log("DEBUG: Total puppets in holder: %d\n", totalPuppets);
        
        // Count how many valid players there are
        for (int i = 0; i < totalPuppets; i++) {
            PuppetInfo* curInfo = Client::getPuppetInfo(i);
            if (curInfo) {
                Logger::log("DEBUG: Puppet %d - connected: %s, sameStage: %s, name: %s\n", 
                    i, curInfo->isConnected ? "yes" : "no", 
                    curInfo->isInSameStage ? "yes" : "no",
                    curInfo->puppetName);
                
                if (curInfo->isConnected && curInfo->isInSameStage) {
                    validPlayerCount++;
                }
            } else {
                Logger::log("DEBUG: Puppet %d is null\n", i);
            }
        }
        
        Logger::log("DEBUG: Valid player count: %d, current spectate index: %d\n", validPlayerCount, mSpectateIndex);

        //Increase or decrease spectate index, followed by clamping it
        int indexDirection = 0;
        if(al::isPadTriggerRight(-1)) {
            indexDirection = 1; //Move index right
            Logger::log("DEBUG: Right trigger pressed!\n");
        }
        if(al::isPadTriggerLeft(-1)) {
            indexDirection = -1; //Move index left  
            Logger::log("DEBUG: Left trigger pressed!\n");
        }

        //Force index to decrease if your current index is higher than valid player count
        if(mSpectateIndex >= validPlayerCount)
            indexDirection = -1;

        //Force index to decrease if your current target changes stages
        if(mSpectateIndex != -1 && indexDirection == 0) {
            // Find the actual puppet info for current spectate index
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
            
            if (!foundValidTarget)
                indexDirection = -1; //Move index left
        }

        //Loop over indexes until you find a suitable one in the same stage
        if (indexDirection != 0) {
            bool isFinalIndex = false;
            while(!isFinalIndex) {
                mSpectateIndex += indexDirection;

                // Start by clamping the index (-1 = self spectate)
                if(mSpectateIndex < -1) mSpectateIndex = validPlayerCount - 1;
                if(mSpectateIndex >= validPlayerCount) mSpectateIndex = -1;

                // -1 is always valid (self spectate)
                if(mSpectateIndex == -1) {
                    isFinalIndex = true;
                } else {
                    // Check if this index corresponds to a valid player
                    int currentValidIndex = 0;
                    for (int i = 0; i < totalPuppets; i++) {
                        PuppetInfo* curInfo = Client::getPuppetInfo(i);
                        if (curInfo && curInfo->isConnected && curInfo->isInSameStage) {
                            if (currentValidIndex == mSpectateIndex) {
                                isFinalIndex = true;
                                break;
                            }
                            currentValidIndex++;
                        }
                    }
                }
            }
        }
        
        //If no index change is happening, end here
        if(mPrevSpectateIndex == mSpectateIndex)
            return;

        //Apply index to target actor and HUD
        if(mSpectateIndex == -1) {
            spectatePoser->setTargetActor(al::getTransPtr(playerBase));
            if (mModeLayout) {
                // mModeLayout->setSpectateString("Self");
            }
            Logger::log("Now spectating self\n");
        } else {
            // Find the puppet info for the current spectate index
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
                spectatePoser->setTargetActor(&targetInfo->playerPos);
                if (mModeLayout) {
                    // mModeLayout->setSpectateString(targetInfo->puppetName);
                }
                Logger::log("Now spectating player: %s\n", targetInfo->puppetName);
            } else {
                // Fallback to self if we couldn't find the target
                mSpectateIndex = -1;
                spectatePoser->setTargetActor(al::getTransPtr(playerBase));
                Logger::log("Couldn't find target, falling back to self\n");
            }
        }

        mPrevSpectateIndex = mSpectateIndex;
    }
}