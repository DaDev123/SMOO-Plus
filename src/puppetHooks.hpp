#pragma once

#include "hk/hook/Trampoline.h"

#include "al/Library/Sequence/Sequence.h"
#include "al/Library/Stage/StageResourceList.h"

#include "actors/PuppetActor.h"
#include "helpers.hpp"
#include "Library/Scene/SceneUtil.h"
#include "logger.hpp"
#include "puppets/PuppetMain.hpp"
#include "server/captureSync.hpp"
#include "server/Client.hpp"

static HkTrampoline<void, al::ActorInitInfo&, al::PlacementInfo*> initObjHook =
    hk::hook::trampoline([](al::ActorInitInfo& initInfo, al::PlacementInfo* placement) -> void {
        al::Sequence* sequence = Client::getSequence();
        if (sequence) {
            auto scene = sequence->mCurrentScene;

            if (!scene || !scene->mIsAlive || !al::isEqualString(scene->mName.cstr(), "StageScene")) {
                return initObjHook.orig(initInfo, placement);
            }
        }

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
                        PuppetHackActor* dupliActor = createPuppetHackActorFromFactory(initInfo, placement, curPuppet->getInfo(), hackName);
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
                    PuppetHackActor* dupliActor = createPuppetHackActorFromFactory(initInfo, placement, debugPuppet->getInfo(), hackName);
                    if (dupliActor) {
                        debugPuppet->addCapture(dupliActor, hackName);
                    }
                }
            }
        }
        return initObjHook.orig(initInfo, placement);
    });

static HkTrampoline<void, al::Scene*, const al::ActorInitInfo&, const char*> initPuppetActorsHook =
    hk::hook::trampoline([](al::Scene* scene, const al::ActorInitInfo& rootInfo, const char* listName) -> void {
        if (!scene || !al::isEqualString(scene->mName.cstr(), "StageScene")) {
            return initPuppetActorsHook.orig(scene, rootInfo, listName);
        }
        al::StageInfo* stageInfo = al::getStageInfoMap(scene, 0);

        int placementCount = 0;
        al::PlacementInfo rootPlacement = al::PlacementInfo();
        al::tryGetPlacementInfoAndCount(&rootPlacement, &placementCount, stageInfo, "PlayerList");
        // check if default placement count for PlayerList is greater than 0 (so we can use the placement info of the player to init our custom puppet
        // actors)
        if (placementCount > 0) {
            al::PlacementInfo playerPlacement = al::PlacementInfo();
            al::getPlacementInfoByIndex(&playerPlacement, rootPlacement, 0);

            for (size_t i = 0; i < Client::getMaxPlayerCount(); i++) {
                createPuppetActorFromFactory(rootInfo, playerPlacement, false);
            }

            // create a debug puppet for testing purposes
            // createPuppetActorFromFactory(rootInfo, playerPlacement, true);
        }
        initPuppetActorsHook.orig(scene, rootInfo, listName);
    });
