#pragma once

#include "hk/hook/Trampoline.h"

#include "al/Library/Sequence/Sequence.h"
#include "al/Library/Stage/StageResourceList.h"

#include "actors/PuppetActor.h"
#include "helpers.hpp"
#include "Library/Scene/SceneUtil.h"
#include "server/captureSync.hpp"
#include "server/Client.hpp"

static HkTrampoline initObjHook = [](TrampolineStatic(), al::ActorInitInfo& initInfo,
                                     al::PlacementInfo* placement) -> void {
    al::Sequence* sequence = Client::getSequence();
    if (sequence) {
        auto scene = sequence->mCurrentScene;

        if (!scene || !scene->mIsAlive || !al::isEqualString(scene->mName.cstr(), "StageScene")) {
            return orig(initInfo, placement);
        }
    }

    const char* className;

    if (al::tryGetClassName(&className, *placement) && isInCaptureList(className)) {
        int serverMaxPlayers = Client::getMaxPlayerCount();

        for (size_t i = 0; i < serverMaxPlayers - 1; i++) {
            PuppetActor* curPuppet = Client::getPuppet(i);
            if (curPuppet) {
                const char* hackName = tryConvertName(className);

                // make sure we only make as many unique puppet hack actors as needed
                if (!curPuppet->isInCaptureList(hackName)) {
                    PuppetHackActor* dupliActor =
                        createPuppetHackActor(initInfo, placement, curPuppet->getInfo(), hackName);
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
                PuppetHackActor* dupliActor =
                    createPuppetHackActor(initInfo, placement, debugPuppet->getInfo(), hackName);
                if (dupliActor) {
                    debugPuppet->addCapture(dupliActor, hackName);
                }
            }
        }
    }
    return orig(initInfo, placement);
};
