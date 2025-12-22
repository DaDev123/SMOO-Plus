#include "al/Library/Camera/CameraDirector.h"
#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Execute/ExecuteDirector.h"
#include "al/Library/Execute/ExecuteRequestKeeper.h"
#include "al/Library/Execute/ExecuteTable.h"
#include "al/Library/Execute/ExecuteTableHolderDraw.h"
#include "al/Library/Execute/ExecuteTableHolderUpdate.h"
#include "al/Library/Execute/ExecuteTablesImpl.h"
#include "al/Library/LiveActor/ActorFlagFunction.h"
#include "al/Library/LiveActor/ActorInitFunction.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "al/Library/Scene/Scene.h"
#include "al/Library/Scene/SceneUtil.h"

#include "al/Library/Yaml/ByamlIter.h"
#include "al/Library/Yaml/ByamlUtil.h"
#include "al/Library/Yaml/Writer/ByamlWriter.h"

#include "Scene/StageSceneStateServerConfig.hpp"
#include "game/Actors/WorldndBorderKeeper.h"
#include "game/Item/Shine.h"
#include "game/Layout/CoinCounter.h"
#include "game/MapObj/AppearSwitchTimer.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Scene/StageSceneStateOption.h"
#include "game/Scene/StageSceneStatePauseMenu.h"
#include "helpers.hpp"
#include "hk/hook/Trampoline.h"
#include "server/freeze/FreezeTagMode.hpp"
#include "server/hns/HideAndSeekMode.hpp"

#include <cstring>
#include <sys/types.h>

#include "Library/Nerve/Nerve.h"
#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.h"

#include "Scene/StageScene.h"
#include "System/GameDataHolder.h"
#include "server/Client.hpp"

#include "server/gamemode/GameModeManager.hpp"

bool checkpointPatch() {
    if (GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG))
        return false;

    return true;
}

bool comboBtnHook(int port) {
    if (GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG))
        return false;

    if (GameModeManager::instance()
            ->isActive()) {  // only switch to combo if any gamemode is active
        return !al::isPadHoldL(port) && al::isPadTriggerDown(port);
    } else {
        return al::isPadTriggerDown(port);
    }
}

void saveWriteHook(al::ByamlWriter* saveByml) {
    const char* serverIP = Client::getCurrentIP();
    const int serverPort = Client::getCurrentPort();

    if (serverIP) {
        saveByml->addString("ServerIP", serverIP);
    } else {
        saveByml->addString("ServerIP", "127.0.0.1");
    }

    if (serverPort) {
        saveByml->addInt("ServerPort", serverPort);
    } else {
        saveByml->addInt("ServerPort", 0);
    }

    saveByml->pop();
}

bool saveReadHook(int* padRumbleInt, al::ByamlIter const& saveByml, char const* padRumbleKey) {
    const char* serverIP = "";
    int serverPort = 0;

    if (al::tryGetByamlString(&serverIP, saveByml, "ServerIP")) {
        Client::setLastUsedIP(serverIP);
    }

    if (al::tryGetByamlS32(&serverPort, saveByml, "ServerPort")) {
        Client::setLastUsedPort(serverPort);
    }

    return al::tryGetByamlS32(padRumbleInt, saveByml, padRumbleKey);
}

bool registerShineToList(Shine* shineActor) {
    if (shineActor->mShineIdx >= 0) {
        Client::tryRegisterShine(shineActor);
    }

    return al::isAlive(shineActor);
}

// void overrideNerveHook(StageSceneStatePauseMenu* thisPtr, al::Nerve* nrvSet) {

//     if (al::isPadHoldZL(-1)) {
//         al::setNerve(thisPtr, &nrvStageSceneStatePauseMenuServerConfig);
//     } else {
//         al::setNerve(thisPtr, nrvSet);
//     }
// }

void overrideHelpFadeNerve(StageSceneStatePauseMenu* thisPtr) {
    // Set label in menu inside LocalizedData/lang/MessageData/LayoutData/Menu.msbt
    thisPtr->exeServerConfig();
    al::setNerve(thisPtr, &NrvStageSceneStatePauseMenu.ServerConfig);
    return;
}

StageSceneStateServerConfig* sceneStateServerConfig = nullptr;

void initStateHook(StageSceneStatePauseMenu* thisPtr, char const* stateName, al::Scene* host,
                   al::LayoutInitInfo const& initInfo, FooterParts* footer, GameDataHolder* data,
                   bool unkBool) {
    thisPtr->mStateOption =
        new StageSceneStateOption(stateName, host, initInfo, footer, data, unkBool);

    sceneStateServerConfig =
        new StageSceneStateServerConfig("ServerConfig", host, initInfo, footer, data, unkBool);
}

void initNerveStateHook(StageSceneStatePauseMenu* stateParent, StageSceneStateOption* stateOption,
                        al::Nerve const* executingNerve, char const* stateName) {
    al::initNerveState(stateParent, stateOption, executingNerve, stateName);

    al::initNerveState(stateParent, sceneStateServerConfig,
                       &NrvStageSceneStatePauseMenu.ServerConfig, "CustomNerveOverride");
}

// skips starting both coin counters
void startCounterHook(CoinCounter* thisPtr) {
    if (!GameModeManager::instance()->isModeRequireUI()) {
        thisPtr->tryStart();
    }
}

// Simple hook that can be used to override isModeE3 checks to enable/disable certain behaviors
bool modeE3Hook() {
    return GameModeManager::instance()->isModeRequireUI();
}

// Skips ending the play guide layout if a mode is active, since the mode would have already ended
// it
void playGuideEndHook(al::SimpleLayoutAppearWaitEnd* thisPtr) {
    if (!GameModeManager::instance()->isModeRequireUI()) {
        thisPtr->end();
    }
}

// Gravity Hooks

void initHackCapHook(al::LiveActor* cappy) {
    al::initActorPoseTQGSV(cappy);
}

al::PlayerHolder* createTicketHook(StageScene* curScene) {
    // only creates custom gravity camera ticket if hide and seek mode is active
    if (GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK)) {
        al::CameraDirector* director = curScene->getCameraDirector();
        if (director) {
            if (director->mPoserFactory) {
                al::CameraTicket* gravityCamera = director->createCameraFromFactory(
                    "CameraPoserCustom", nullptr, 0, 5, sead::Matrix34f::ident);

                HideAndSeekMode* mode = GameModeManager::instance()->getMode<HideAndSeekMode>();

                mode->setCameraTicket(gravityCamera);
            }
        }
    }

    if (GameModeManager::instance()->isMode(GameMode::FREEZETAG) ||
        GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK)) {
        al::CameraDirector* director = curScene->getCameraDirector();
        if (director && director->mPoserFactory) {
            al::CameraTicket* spectateCamera = director->createCameraFromFactory(
                "CameraPoserActorSpectate", nullptr, 0, 5, sead::Matrix34f::ident);

            if (GameModeManager::instance()->isMode(GameMode::FREEZETAG)) {
                FreezeTagMode* mode = GameModeManager::instance()->getMode<FreezeTagMode>();
                mode->setCameraTicket(spectateCamera);
            } else if (GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK)) {
                HideAndSeekMode* mode = GameModeManager::instance()->getMode<HideAndSeekMode>();
                mode->setCameraTicket(spectateCamera);
            }
        }
    }

    return al::getScenePlayerHolder(curScene);
}

bool borderPullBackHook(WorldEndBorderKeeper* thisPtr) {
    bool isFirstStep = al::isFirstStep(thisPtr);

    if (isFirstStep) {
        if (GameModeManager::instance()->isModeAndActive(GameMode::HIDEANDSEEK)) {
            HideAndSeekMode* mode = GameModeManager::instance()->getMode<HideAndSeekMode>();

            if (mode->isUseGravity()) {
                killMainPlayer(thisPtr->mActor);
            }
        }
    }

    return isFirstStep;
}

constexpr static al::ExecuteTable DrawTableCustom[] = {
    createDrawTable("OnlineDrawExecutors", "PuppetActor", "ActorModelDrawDeferred", "PuppetActor",
                    "ActorModelDrawDeferred")};

constexpr static al::ExecuteTable UpdateTableCustom[] = {
    createUpdateTable("OnlineUpdateExecutors", "PuppetActor", "PuppetActor"),
};

static HkTrampoline<void, al::ExecuteDirector*, const al::ExecuteSystemInitInfo&> drawTableHook =
    hk::hook::trampoline([](al::ExecuteDirector* director,
                            const al::ExecuteSystemInitInfo& initInfo) -> void {
        drawTableHook.orig(director, initInfo);

        constexpr s32 UpdateTableSize = sizeof(UpdateTableCustom) / sizeof(UpdateTableCustom[0]);
        al::ExecuteTableHolderUpdate** updateTables =
            new al::ExecuteTableHolderUpdate*[director->mUpdateTableCount + UpdateTableSize]();

        for (s32 i = 0; i < director->mUpdateTableCount; i++) {
            updateTables[i] = director->mUpdateTables[i];
        }
        for (s32 i = 0; i < UpdateTableSize; i++) {
            updateTables[director->mUpdateTableCount + i] = new al::ExecuteTableHolderUpdate();
            const al::ExecuteTable& curTable = UpdateTableCustom[i];
            updateTables[director->mUpdateTableCount + i]->init(
                curTable.name, initInfo, curTable.executeOrders, curTable.executeOrderCount);
        }
        director->mUpdateTableCount += UpdateTableSize;
        director->mUpdateTables = updateTables;

        constexpr s32 DrawTableSize = sizeof(DrawTableCustom) / sizeof(DrawTableCustom[0]);
        al::ExecuteTableHolderDraw** drawTables =
            new al::ExecuteTableHolderDraw*[director->mDrawTableCount + DrawTableSize]();
        for (s32 i = 0; i < director->mDrawTableCount; i++) {
            drawTables[i] = director->mDrawTables[i];
        }
        for (s32 i = 0; i < DrawTableSize; i++) {
            drawTables[director->mDrawTableCount + i] = new al::ExecuteTableHolderDraw();
            const al::ExecuteTable& curTable = DrawTableCustom[i];
            drawTables[director->mDrawTableCount + i]->init(
                curTable.name, initInfo, curTable.executeOrders, curTable.executeOrderCount);
        }
        director->mDrawTableCount += DrawTableSize;
        director->mDrawTables = drawTables;
    });

void updateStateHook(al::Scene* scene) {
    al::executeUpdateList(scene->mLayoutKit, "OnlineUpdateExecutors", "PuppetActor");
    rs::updateEffectSystemEnv(scene);
}

void updateDrawHook(al::ExecuteDirector* thisPtr, const char* listName, const char* kit) {
    thisPtr->drawList("OnlineDrawExecutors", "PuppetActor");

    Logger::log("Updating Draw List for: %s %s\n", listName, kit);
    thisPtr->drawList(listName, kit);
}

void exeWaitHook(StageSceneStatePauseMenu* thisPtr) {
    if (al::isFirstStep(thisPtr)) {
        thisPtr->mSelectParts->setSelectMessage(2, u"Mod Config");
    }
}