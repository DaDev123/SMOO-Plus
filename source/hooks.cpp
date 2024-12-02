#include <sys/types.h>

#include "al/byaml/ByamlIter.h"
#include "al/byaml/writer/ByamlWriter.h"
#include "al/execute/ExecuteDirector.h"
#include "al/execute/ExecuteOrder.h"
#include "al/execute/ExecuteTable.h"
#include "al/execute/ExecuteTableHolderDraw.h"
#include "al/execute/ExecuteTableHolderUpdate.h"
#include "al/scene/Scene.h"
#include "al/util.hpp"
#include "al/util/GraphicsUtil.h"
#include "al/util/KitUtil.h"

#include "game/Actors/WorldEndBorderKeeper.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/StageScene/StageSceneStateOption.h"
#include "game/StageScene/StageSceneStatePauseMenu.h"
#include "game/StageScene/StageSceneStateServerConfig.hpp"

#include "logger.hpp"

#include "main.hpp"

#include "rs/util.hpp"
#include "rs/util/InputUtil.h"

#include "sead/basis/seadNew.h"
#include "sead/math/seadVector.h"
#include "sead/prim/seadSafeString.h"

#include "server/Client.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeManager.hpp"

bool comboBtnHook(int port) {
    GameModeManager* gmm = GameModeManager::instance();

    // only switch to combo if the gamemode is active
    if (!gmm->isActive()) {
        return al::isPadTriggerDown(port);
    }

    // only if the gamemode wants it
    GameModeBase* mode = gmm->getMode<GameModeBase>();
    if (!mode || mode->ignoreComboBtn()) {
        return false;
    }

    return !al::isPadHoldL(port) && al::isPadTriggerDown(port);
}

void saveWriteHook(al::ByamlWriter* saveByml) {
    const char* serverIP     = Client::getCurrentIP();
    const int   serverPort   = Client::getCurrentPort();
    const bool  serverHidden = Client::isServerHidden();

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

    saveByml->addBool("ServerHidden", serverHidden);

    saveByml->pop();
}

bool saveReadHook(int* padRumbleInt, al::ByamlIter const& saveByml, char const* padRumbleKey) {
    const char* serverIP     = "";
    int         serverPort   = 0;
    bool        serverHidden = false;

    if (al::tryGetByamlString(&serverIP, saveByml, "ServerIP")) {
        Client::setLastUsedIP(serverIP);
    }

    if (al::tryGetByamlS32(&serverPort, saveByml, "ServerPort")) {
        Client::setLastUsedPort(serverPort);
    }

    if (al::tryGetByamlBool(&serverHidden, saveByml, "ServerHidden")) {
        Client::setServerHidden(serverHidden);
    }

    return al::tryGetByamlS32(padRumbleInt, saveByml, padRumbleKey);
}

bool registerShineToList(Shine* shineActor) {
    if (shineActor->mShineIdx >= 0) {
        Client::tryRegisterShine(shineActor);
    }

    return al::isAlive(shineActor);
}

void overrideHelpFadeNerve(StageSceneStatePauseMenu* thisPtr) {
    // Set label in menu inside LocalizedData/${lang}/MessageData/LayoutMessage.szs/Menu.msbt/Menu_Help
    thisPtr->exeServerConfig();
    al::setNerve(thisPtr, &nrvStageSceneStatePauseMenuServerConfig);
}

StageSceneStateServerConfig* sceneStateServerConfig = nullptr;

void initStateHook(
    StageSceneStatePauseMenu* thisPtr,
    char const* stateName,
    al::Scene* host,
    al::LayoutInitInfo const& initInfo,
    FooterParts* footer,
    GameDataHolder* data,
    bool unkBool
) {
    thisPtr->mStateOption = new StageSceneStateOption(stateName, host, initInfo, footer, data, unkBool);

    sceneStateServerConfig = new StageSceneStateServerConfig("ServerConfig", host, initInfo, footer, data, unkBool);
}

void initNerveStateHook(
    StageSceneStatePauseMenu* stateParent,
    StageSceneStateOption* stateOption,
    al::Nerve const* executingNerve,
    char const* stateName
) {
    al::initNerveState(stateParent, stateOption, executingNerve, stateName);

    al::initNerveState(stateParent, sceneStateServerConfig, &nrvStageSceneStatePauseMenuServerConfig, "CustomNerveOverride");
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

// Skips ending the play guide layout if a mode is active, since the mode would have already ended it
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
    // only creates custom camera ticket if the mode wants it
    GameModeBase* mode = GameModeManager::instance()->getMode<GameModeBase>();
    if (mode && mode->hasCustomCamera()) {
        al::CameraDirector* director = curScene->getCameraDirector();
        if (director && director->mFactory) {
            mode->createCustomCameraTicket(director);
        }
    }

    return al::getScenePlayerHolder(curScene);
}

bool borderPullBackHook(WorldEndBorderKeeper* thisPtr) {
    bool isFirstStep = al::isFirstStep(thisPtr);

    if (isFirstStep && GameModeManager::instance()->isActive()) {
        GameModeBase* mode = GameModeManager::instance()->getMode<GameModeBase>();
        if (mode) {
            mode->onBorderPullBackFirstStep(thisPtr->mActor);
        }
    }

    return isFirstStep;
}

void drawTableHook(al::ExecuteDirector* thisPtr, const al::ExecuteSystemInitInfo& initInfo) {
    thisPtr->mUpdateTableCount = updateTableSize;
    thisPtr->mUpdateTables     = new al::ExecuteTableHolderUpdate*[thisPtr->mUpdateTableCount]();

    for (int i = 0; i < thisPtr->mUpdateTableCount; i++) {
        thisPtr->mUpdateTables[i] = new al::ExecuteTableHolderUpdate();
        const al::ExecuteTable& curTable = updateTable[i];
        // Logger::log("Update Table Name: %s Count: %d\n", curTable.mName, curTable.mExecuteOrderCount);
        thisPtr->mUpdateTables[i]->init(curTable.mName, initInfo, curTable.mExecuteOrders, curTable.mExecuteOrderCount);
    }

    thisPtr->mDrawTableCount = drawTableSize;
    thisPtr->mDrawTables     = new al::ExecuteTableHolderDraw*[thisPtr->mDrawTableCount]();

    for (int i = 0; i < thisPtr->mDrawTableCount; i++) {
        thisPtr->mDrawTables[i] = new al::ExecuteTableHolderDraw();
        const al::ExecuteTable* curTable = &drawTable[i];
        // Logger::log("Draw Table Name: %s Count: %d\n", curTable->mName, curTable->mExecuteOrderCount);
        thisPtr->mDrawTables[i]->init(curTable->mName, initInfo, curTable->mExecuteOrders, curTable->mExecuteOrderCount);
    }

    thisPtr->mRequestKeeper = new al::ExecuteRequestKeeper(thisPtr->mRequestMax);
}

void updateStateHook(al::Scene* scene) {
    al::executeUpdateList(scene->mActorKit, "OnlineUpdateExecutors", "PuppetActor");
    rs::updateEffectSystemEnv(scene);
}

void updateDrawHook(al::ExecuteDirector* thisPtr, const char* listName, const char* kit) {
    thisPtr->drawList("OnlineDrawExecutors", "PuppetActor");

    Logger::log("Updating Draw List for: %s %s\n", listName, kit);
    thisPtr->drawList(listName, kit);
}
