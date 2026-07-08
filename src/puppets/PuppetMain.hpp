#pragma once
#include "hk/diag/diag.h"

#include "al/Library/LiveActor/ActorFactory.h"
#include "al/Library/Memory/HeapUtil.h"
#include "al/Library/Placement/PlacementFunction.h"
#include "al/Library/Scene/SceneUtil.h"

#include "actors/PuppetActor.h"
#include "heap/seadHeapMgr.h"
#include "server/Client.hpp"

inline al::LiveActor* createPuppetActorFromFactory(const al::ActorInitInfo& initInfo) {
    sead::ScopedCurrentHeapSetter setter(al::getSceneHeap());

    PuppetActor* newActor = new PuppetActor("PuppetActor");

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

    return newActor;
}
