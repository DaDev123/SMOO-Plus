#pragma once
#include "hk/mem/BssHeap.h"

#include "al/Library/LiveActor/ActorFactory.h"
#include "al/Library/Placement/PlacementFunction.h"
#include "al/Library/Scene/SceneUtil.h"

#include "actors/PuppetActor.h"
#include "logger.hpp"
#include "server/Client.hpp"

inline auto logHakkunHeapUsage = []() -> void {
    size size = hk::mem::sMainHeap.getTotalSize() - hk::mem::sMainHeap.getFreeSize();
    Logger::log("Hakkun Heap Usage: %zu\n", size);
};

inline al::LiveActor* createPuppetActorFromFactory(const al::ActorInitInfo& initInfo, bool isDebug) {
    Logger::log("creating actor\n");
    logHakkunHeapUsage();
    PuppetActor* newActor = new (Client::sInstance->mHakkunSceneHeap) PuppetActor("PuppetActor");
    Logger::log("created actor\n");
    logHakkunHeapUsage();

    if (!isDebug) {
        if (Client::tryAddPuppet(newActor)) {
            PuppetInfo* curInfo = Client::getLatestInfo();
            if (!curInfo) {
                Logger::log("[Factory] ERROR: Puppet Info is Null!\n");
                delete newActor;
                return nullptr;
            } else {
                Logger::log("[Factory] Creating puppet for player: %s\n", curInfo->puppetName);

                // set puppet info first before calling init so we can get costume info from the
                // info
                newActor->initOnline(curInfo);
                newActor->init(initInfo);

                Logger::log("[Factory] Puppet initialized successfully for %s\n", curInfo->puppetName);
            }
        } else {
            Logger::log("[Factory] ERROR: Failed to add puppet to client\n");
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
        newActor->init(initInfo);
        newActor->mIsDebug = true;
        newActor->makeActorAlive();

        if (Client::tryAddDebugPuppet(newActor)) {
            // Logger::log("[Factory] Debug Puppet Created Successfully!\n");
        } else {
            // Logger::log("[Factory] WARNING: Failed to register debug puppet\n");
        }
    }

    return newActor;
}
