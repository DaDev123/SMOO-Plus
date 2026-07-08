#pragma once

#include "al/Library/LiveActor/ActorInitUtil.h"
#include "al/Library/Placement/PlacementFunction.h"
#include "al/Library/Placement/PlacementInfo.h"

#include "actors/PuppetHackActor.h"
#include "algorithms/CaptureTypes.h"
#include "heap/seadHeapMgr.h"
#include "Library/Memory/HeapUtil.h"
#include "server/Client.hpp"

// Helper Methods
static bool isInCaptureList(const char* capture) {
    return CaptureTypes::FindType(capture) != CaptureTypes::Type::Unknown;
}

static PuppetHackActor* createPuppetHackActor(const al::ActorInitInfo& initInfo,
                                              const al::PlacementInfo* placementInfo, PuppetInfo* curInfo,
                                              const char* hackType) {
    sead::ScopedCurrentHeapSetter setter(al::getSceneHeap());

    int serverMaxPlayers = Client::getMaxPlayerCount();  // TODO: Find a way around needing to do this, such
                                                         // as creating a single hack actor per puppet that
                                                         // can dynamically switch models

    // only use this if player count is >= 7
    // if (serverMaxPlayers >= 7) {
    //     const char* stageName = "";
    //     if (placementInfo->mPlacementIter.tryGetStringByKey(&stageName, "PlacementFileName")) {
    //         if (al::isEqualString(stageName, "ForestWorldHomeStage")) {
    //             return nullptr;
    //         }
    //     }
    // }

    if (serverMaxPlayers > 10) {  // disable capture sync if dealing with more than 10 players
        return nullptr;
    }

    PuppetHackActor* newActor = new PuppetHackActor("PuppetHackActor");

    newActor->initOnline(curInfo, hackType);  // set puppet info first before calling init so we
                                              // can get costume info from the info

    newActor->init(initInfo);

    return newActor;
}
