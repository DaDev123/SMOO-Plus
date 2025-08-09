#include "server/ExtrasCode.hpp"
#include "al/util.hpp" // Passe die Includes an, je nachdem wo deine Hilfsfunktionen liegen
#include <cmath>
#include <string>
#include "packets/Extras.hpp"
#include "helpers/helpers/GetHelper.h"

//------------------------------usefull includes---------------------------------
#include "game/GameData/GameDataHolderAccessor.h"            //for health and coins
#include "game/GameData/GameDataFunction.h"
#include "game/HakoniwaSequence/HakoniwaSequence.h"
#include "game/Player/PlayerActorBase.h"                    //mario base
#include "game/Player/PlayerActorHakoniwa.h"                //Mario hakoniwa
#include "game/Player/PlayerFunction.h"                     //mario function
#include "game/Player/PlayerHackKeeper.h"                   //mario hack keeper
#include "game/Player/PlayerCostumeInfo.h"      	        //for coszume names
#include "game/Player/PlayerHitPointData.h"                 //for life 
#include "game/Player/HackCap/PlayerCapActionHistory.h"     //for infinite cap Dives
#include "game/Player/Actions/PlayerWallActionHistory.h"    //for infinite cap Dives/wall jumps
#include "game/StageScene/StageScene.h"



void handleNoclip(PlayerActorHakoniwa* hakoniwa, bool gNoclip, bool isYukimaru) {
    if (hakoniwa && !isYukimaru) {
        static bool wasNoclipOn = false;
        
        // Transitioning FROM noclip TO normal
        if (!gNoclip && wasNoclipOn) {
            al::onCollide(hakoniwa);
            hakoniwa->endDemoPuppetable();
            al::setVelocityZero(hakoniwa); // Clear any remaining velocity
        }
        // Transitioning FROM normal TO noclip  
        else if (gNoclip && !wasNoclipOn) {
            hakoniwa->startDemoPuppetable();
            hakoniwa->mPlayerAnimator->startAnim("Wait");
            hakoniwa->exeJump();
            al::offCollide(hakoniwa);
        }
        
        // Currently in noclip mode - handle movement
        if (gNoclip) {
            static float speed = 20.0f;
            static float speedMax = 250.0f;
            static float vspeed = 10.0f;
            static float speedGain = 0.0f;
            
            sead::Vector3f *playerPos = al::getTransPtr(hakoniwa);
            sead::Vector3f *cameraPos = al::getCameraPos(hakoniwa, 0);
            sead::Vector2f *leftStick = al::getLeftStick(-1);
            
            al::setVelocityZero(hakoniwa);
            
            // Your movement code here...
            float d = sqrt(al::powerIn(playerPos->x - cameraPos->x, 2) + (al::powerIn(playerPos->z - cameraPos->z, 2)));
            float vx = ((speed + speedGain) / d) * (playerPos->x - cameraPos->x);
            float vz = ((speed + speedGain) / d) * (playerPos->z - cameraPos->z);
            playerPos->x -= leftStick->x * vz;
            playerPos->z += leftStick->x * vx;
            playerPos->x += leftStick->y * vx;
            playerPos->z += leftStick->y * vz;
            
            if (al::isPadHoldX(-1) || al::isPadHoldY(-1)) speedGain += 0.5f;
            if (al::isPadHoldA(-1) || al::isPadHoldB(-1)) speedGain -= 0.5f;
            if (speedGain <= 0.0f) speedGain = 0.0f;
            if (speedGain >= speedMax) speedGain = speedMax;
            if (al::isPadHoldZL(-1)) playerPos->y -= (vspeed + speedGain / 3);
            if (al::isPadHoldZR(-1)) playerPos->y += (vspeed + speedGain / 3);
        }
        
        wasNoclipOn = gNoclip;
    }
}



void handleInfiniteCapBounce(PlayerActorHakoniwa* playerBase, bool gInfiniteCapBounce) {

    //static int capBounceFrameCounter = 0;   //für die frames
    //static int x = 3;                       //für die frames

    //if (playerBase && gInfiniteCapBounce) {
    //    capBounceFrameCounter++;
    //    if (capBounceFrameCounter >= x) { // alle 3 Frames
    //        PlayerActorHakoniwa* hakoniwa = static_cast<PlayerActorHakoniwa*>(playerBase);
    //        if (hakoniwa && hakoniwa->mHackCap && hakoniwa->mHackCap->mCapActionHistory && hakoniwa->mPlayerWallActionHistory) {    //wenn hakoniwa und hackcap und capactionhistory und playerwallactionhistory
    //            hakoniwa->mHackCap->mCapActionHistory->clearCapJump();  //cap jump reset
    //            hakoniwa->mPlayerWallActionHistory->reset();            //wall jump reset
    //        }
    //        capBounceFrameCounter = 0; // zurücksetzen
    //    }
    //}
}


void giveLifeUpHeart(PlayerActorHakoniwa* hakoniwa) {
    if (!hakoniwa) return;
    
    // Get GameDataHolderAccessor from the actor
    GameDataHolderAccessor accessor(hakoniwa);
    if (accessor.mData && accessor.mData->mGameDataFile) {
        PlayerHitPointData* hitData = accessor.mData->mGameDataFile->getPlayerHitPointData();
        if (hitData) {
            hitData->mIsHaveMaxUpItem = true;
            hitData->mCurrentHit = 6;
        }
    }
}


void setOutfit(PlayerActorHakoniwa* hakoniwa, const char* body, const char* cap) {

    // sets the current worn costume
   // static void wearCostume(GameDataHolderWriter, char const *);

   // // sets the current worn cap
   // static void wearCap(GameDataHolderWriter, char const*);
    if (!hakoniwa) return;

    // Get the current scene
    al::Scene* scene = tryGetScene();
    if (!scene) return;

    // Create actor init info
    al::ActorInitInfo initInfo;
    al::initActorInitInfo(&initInfo, scene, nullptr, nullptr, nullptr, nullptr, nullptr);

    // Set the player model using the existing function
    PlayerFunction::initMarioModelActor(
        hakoniwa, 
        initInfo, 
        body, 
        cap, 
        nullptr, 
        false
    );

    // Send costume info to other players
    extern void sendCostumeInfPacket(const char* body, const char* cap);
    sendCostumeInfPacket(body, cap);
    GameDataHolder* gameDataHolder = tryGetGameDataHolder();
    if (gameDataHolder) {
        GameDataHolderWriter writer(gameDataHolder);
        GameDataFunction::wearCostume(writer, body);
        GameDataFunction::wearCap(writer, cap);
    }
}

