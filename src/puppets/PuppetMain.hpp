#pragma once
#include "hk/diag/diag.h"
#include "hk/mem/BssHeap.h"

#include "al/Library/LiveActor/ActorFactory.h"
#include "al/Library/Placement/PlacementFunction.h"
#include "al/Library/Scene/SceneUtil.h"

#include "actors/PuppetActor.h"
#include "server/Client.hpp"

inline al::LiveActor* createPuppetActorFromFactory(const al::ActorInitInfo& initInfo, bool isDebug) {
    PuppetActor* newActor = new (Client::sInstance->mHakkunSceneHeap) PuppetActor("PuppetActor");
    if (!isDebug) {
        if (Client::tryAddPuppet(newActor)) {
            PuppetInfo* curInfo = Client::getLatestInfo();
            if (!curInfo) {
                hk::diag::logLine("[Factory] ERROR: Puppet Info is Null!");
                delete newActor;
                return nullptr;
            } else {
                hk::diag::logLine("[Factory] Creating puppet for player: %s", curInfo->puppetName);

                // set puppet info first before calling init so we can get costume info from the
                // info
                newActor->initOnline(curInfo);
                newActor->init(initInfo);

                hk::diag::logLine("[Factory] Puppet initialized successfully for %s", curInfo->puppetName);
            }
        } else {
            hk::diag::logLine("[Factory] ERROR: Failed to add puppet to client");
            delete newActor;
            return nullptr;
        }
    } else {
        // hk::diag::logLine("[Factory] Creating Debug/Test Puppet");

        PuppetInfo* debugInfo = Client::getDebugPuppetInfo();
        if (!debugInfo) {
            // hk::diag::logLine("[Factory] ERROR: Debug puppet info is null!");
            delete newActor;
            return nullptr;
        }

        newActor->initOnline(debugInfo);
        newActor->init(initInfo);
        newActor->mIsDebug = true;
        newActor->makeActorAlive();

        if (Client::tryAddDebugPuppet(newActor)) {
            // hk::diag::logLine("[Factory] Debug Puppet Created Successfully!");
        } else {
            // hk::diag::logLine("[Factory] WARNING: Failed to register debug puppet");
        }
    }

    return newActor;
}
