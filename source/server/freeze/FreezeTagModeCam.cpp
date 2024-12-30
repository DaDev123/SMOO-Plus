#include "server/freeze/FreezeTagMode.hpp"
#include "cameras/CameraPoserActorSpectate.h"

void FreezeTagMode::updateSpectateCam(PlayerActorBase* playerBase)
{
    //If the specate camera ticket is active, get the camera poser
    al::CameraPoser* curPoser;
    al::CameraDirector* director = mCurScene->getCameraDirector();

    if (director) {
        al::CameraPoseUpdater* updater = director->getPoseUpdater(0);
        if (updater && updater->mTicket) {
            curPoser = updater->mTicket->mPoser;
        }
    }
    
    //Verify 100% that this poser is the actor spectator
    if (al::isEqualString(curPoser->getName(), "CameraPoserActorSpectate")) {
        cc::CameraPoserActorSpectate* spectatePoser = (cc::CameraPoserActorSpectate*)curPoser;
        spectatePoser->setPlayer(playerBase);

        //Increase or decrease spectate index, followed by clamping it
        int indexDirection = 0;
        if(al::isPadTriggerRight(-1) && !mIsEndgameActive) indexDirection = 1; //Move index right
        if(al::isPadTriggerLeft(-1) && !mIsEndgameActive) indexDirection = -1; //Move index left

        //Force index to decrease if your current index is higher than runner player count
        //Force index towards -1 during endgame if spectate index is not already -1
        if(mSpectateIndex >= mInfo->mRunnerPlayers.size() || (mIsEndgameActive && mSpectateIndex != -1))
            indexDirection = -1;

        //Force index to decrease if your current target changes stages
        if(mSpectateIndex != -1 && indexDirection == 0)
            if(!mInfo->mRunnerPlayers.at(mSpectateIndex)->isInSameStage)
                indexDirection = -1; //Move index left

        //Loop over indexs until you find a sutible one in the same stage
        bool isFinalIndex = false;
        while(!isFinalIndex) {
            mSpectateIndex += indexDirection;

            // Start by clamping the index
            if(mSpectateIndex < -1) mSpectateIndex = mInfo->mRunnerPlayers.size() - 1;
            if(mSpectateIndex >= mInfo->mRunnerPlayers.size()) mSpectateIndex = -1;

            // If not in same stage, skip
            if(mSpectateIndex != -1) {
                if(mInfo->mRunnerPlayers.at(mSpectateIndex)->isInSameStage)
                    isFinalIndex = true;
            } else {
                isFinalIndex = true;
            }

        }
        
        //If no index change is happening, end here
        if(mPrevSpectateIndex == mSpectateIndex)
            return;

        //Apply index to target actor and HUD
        if(mSpectateIndex == -1) {
            spectatePoser->setTargetActor(al::getTransPtr(playerBase));
            mModeLayout->setSpectateString("Spectate");
        } else {
            spectatePoser->setTargetActor(&mInfo->mRunnerPlayers.at(mSpectateIndex)->playerPos);
            mModeLayout->setSpectateString(mInfo->mRunnerPlayers.at(mSpectateIndex)->puppetName);
        }

        mPrevSpectateIndex = mSpectateIndex;
    }
}