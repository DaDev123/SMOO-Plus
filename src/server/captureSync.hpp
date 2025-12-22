#pragma once

#include "al/Library/LiveActor/ActorFactory.h"
#include "al/Library/LiveActor/ActorInitUtil.h"
#include "al/Library/Placement/PlacementFunction.h"
#include "al/Library/Placement/PlacementInfo.h"

#include "actors/PuppetHackActor.h"
#include "algorithms/CaptureTypes.h"
#include "server/Client.hpp"

// Helper Methods
static bool isInCaptureList(const char* capture) {
    return CaptureTypes::FindType(capture) != CaptureTypes::Type::Unknown;
}

static PuppetHackActor* createPuppetHackActorFromFactory(const al::ActorInitInfo& rootInitInfo, const al::PlacementInfo* rootPlacementInfo, PuppetInfo* curInfo,
                                                         const char* hackType) {
    al::ActorInitInfo actorInitInfo = al::ActorInitInfo();
    actorInitInfo.initViewIdSelf(rootPlacementInfo, rootInitInfo);

    int serverMaxPlayers = Client::getMaxPlayerCount();  // TODO: Find a way around needing to do this, such as
                                                         // creating a single hack actor per puppet that can
                                                         // dynamically switch models

    // only use this if player count is 8
    if (serverMaxPlayers == 8) {
        const char* stageName = "";
        if (actorInitInfo.placementInfo->mPlacementIter.tryGetStringByKey(&stageName, "PlacementFileName")) {
            if (al::isEqualString(stageName, "ForestWorldHomeStage")) {
                return nullptr;
            }
        }
    }

    if (serverMaxPlayers > 8) {  // disable capture sync if dealing with more than 8 players
        return nullptr;
    }

    al::ActorCreatorFunction createActor = actorInitInfo.actorFactory->getCreator("PuppetHackActor");

    if (createActor) {
        PuppetHackActor* newActor = (PuppetHackActor*)createActor("PuppetHackActor");

        newActor->initOnline(curInfo, hackType);  // set puppet info first before calling init so we
                                                  // can get costume info from the info

        newActor->init(actorInitInfo);

        return newActor;
    } else {
        return nullptr;
    }
}
