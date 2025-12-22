#include "actors/PuppetActor.h"
#include "actors/PuppetHackActor.h"
#include "al/Library/LiveActor/ActorFactory.h"
#include "al/Library/LiveActor/ActorInitUtil.h"
#include "al/Library/Placement/PlacementFunction.h"
#include "al/Library/Placement/PlacementInfo.h"
#include "algorithms/CaptureTypes.h"
#include "helpers.hpp"
#include "server/Client.hpp"

// Helper Methods

bool isInCaptureList(const char* capture) {
    return CaptureTypes::FindType(capture) != CaptureTypes::Type::Unknown;
}

PuppetHackActor* createPuppetHackActorFromFactory(al::ActorInitInfo const& rootInitInfo,
                                                  const al::PlacementInfo* rootPlacementInfo,
                                                  PuppetInfo* curInfo, const char* hackType) {
    al::ActorInitInfo actorInitInfo = al::ActorInitInfo();
    actorInitInfo.initViewIdSelf(rootPlacementInfo, rootInitInfo);

    int serverMaxPlayers =
        Client::getMaxPlayerCount();  // TODO: Find a way around needing to do this, such as
                                      // creating a single hack actor per puppet that can
                                      // dynamically switch models

    // only use this if player count is 8
    if (serverMaxPlayers == 8) {
        const char* stageName = "";
        if (actorInitInfo.placementInfo->mPlacementIter.tryGetStringByKey(&stageName,
                                                                          "PlacementFileName")) {
            if (al::isEqualString(stageName, "ForestWorldHomeStage")) {
                return nullptr;
            }
        }
    }

    if (serverMaxPlayers > 8) {  // disable capture sync if dealing with more than 8 players
        return nullptr;
    }

    al::ActorCreatorFunction createActor =
        actorInitInfo.actorFactory->getCreator("PuppetHackActor");

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

// Hooks

al::LiveActor* initObjHook(al::ActorInitInfo const& initInfo, al::PlacementInfo const* placement) {
    al::ActorInitInfo newInfo = al::ActorInitInfo();
    newInfo.initViewIdSelf(placement, initInfo);

    const char* className = "";
    al::tryGetClassName(&className, newInfo);

    if (isInCaptureList(className)) {
        int serverMaxPlayers = Client::getMaxPlayerCount();

        for (size_t i = 0; i < serverMaxPlayers - 1; i++) {
            PuppetActor* curPuppet = Client::getPuppet(i);
            if (curPuppet) {
                const char* hackName = tryConvertName(className);

                // make sure we only make as many unique puppet hack actors as needed
                if (!curPuppet->isInCaptureList(hackName)) {
                    PuppetHackActor* dupliActor = createPuppetHackActorFromFactory(
                        initInfo, placement, curPuppet->getInfo(), hackName);
                    if (dupliActor) {
                        curPuppet->addCapture(dupliActor, hackName);
                    }
                }
            }
        }

        PuppetActor* debugPuppet = Client::getDebugPuppet();

        if (debugPuppet) {
            const char* hackName = tryConvertName(className);
            if (!debugPuppet->isInCaptureList(hackName)) {
                PuppetHackActor* dupliActor = createPuppetHackActorFromFactory(
                    initInfo, placement, debugPuppet->getInfo(), hackName);
                if (dupliActor) {
                    debugPuppet->addCapture(dupliActor, hackName);
                }
            }
        }
    }

    return al::createPlacementActorFromFactory(initInfo, placement);
}
