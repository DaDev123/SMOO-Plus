#pragma once
#include "al/Library/LiveActor/ActorFactory.h"
#include "al/Library/Placement/PlacementFunction.h"
#include "al/Library/Placement/PlacementInfo.h"
#include "al/Library/Scene/SceneUtil.h"

#include "server/Client.hpp"

static al::LiveActor* createPuppetActorFromFactory(al::ActorInitInfo const& rootInitInfo, al::PlacementInfo const& rootPlacementInfo, bool isDebug) {
    al::ActorInitInfo actorInitInfo = al::ActorInitInfo();
    actorInitInfo.initViewIdSelf(&rootPlacementInfo, rootInitInfo);

    al::ActorCreatorFunction createActor = actorInitInfo.actorFactory->getCreator("PuppetActor");

    if (createActor) {
        PuppetActor* newActor = (PuppetActor*)createActor("PuppetActor");

        if (!isDebug) {
            if (Client::tryAddPuppet(newActor)) {
                PuppetInfo* curInfo = Client::getLatestInfo();
                if (!curInfo) {
                    // Logger::log("[Factory] ERROR: Puppet Info is Null!\n");
                    delete newActor;
                    return nullptr;
                } else {
                    // Logger::log("[Factory] Creating puppet for player: %s\n", curInfo->puppetName);

                    // set puppet info first before calling init so we can get costume info from the
                    // info
                    newActor->initOnline(curInfo);
                    newActor->init(actorInitInfo);

                    // Logger::log("[Factory] Puppet initialized successfully for %s\n", curInfo->puppetName);
                }
            } else {
                // Logger::log("[Factory] ERROR: Failed to add puppet to client\n");
                delete newActor;
                return nullptr;
            }
        } else {
            // Logger::log("[Factory] Creating Debug/Test Puppet\n");

            PuppetInfo* debugInfo = Client::getDebugPuppetInfo();
            if (!debugInfo) {
                // Logger::log("[Factory] ERROR: Debug puppet info is null!\n");
                delete newActor;
                return nullptr;
            }

            newActor->initOnline(debugInfo);
            newActor->init(actorInitInfo);
            newActor->mIsDebug = true;
            newActor->makeActorAlive();

            if (Client::tryAddDebugPuppet(newActor)) {
                // Logger::log("[Factory] Debug Puppet Created Successfully!\n");
            } else {
                // Logger::log("[Factory] WARNING: Failed to register debug puppet\n");
            }
        }

        return newActor;
    } else {
        // Logger::log("[Factory] ERROR: Could not find PuppetActor creator in factory!\n");
        return nullptr;
    }
}
