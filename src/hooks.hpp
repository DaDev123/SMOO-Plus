#include "hk/hook/Trampoline.h"

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

#include "game/Actors/WorldndBorderKeeper.h"
#include "game/Item/Shine.h"
#include "game/Layout/CoinCounter.h"
#include "game/MapObj/AppearSwitchTimer.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Scene/StageSceneStateOption.h"
#include "game/Scene/StageSceneStatePauseMenu.h"

#include <cstring>
#include <sys/types.h>

#include "helpers.hpp"
#include "layouts/ConnectionStatus.h"
#include "Library/Collision/CollisionPartsTriangle.h"
#include "Library/Nerve/Nerve.h"
#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.h"
#include "Scene/StageScene.h"
#include "Scene/StageSceneStateServerConfig.hpp"
#include "server/Client.hpp"
#include "server/freeze/FreezeTagMode.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/hns/HideAndSeekMode.hpp"
#include "System/GameDataHolder.h"
#include "TwistsConfig.hpp"

bool checkpointPatch() {
    if (GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG))
        return false;

    return true;
}

static HkReplace<bool, al::IUseSceneObjHolder*> comboBtnHook = hk::hook::replace([](al::IUseSceneObjHolder* holder) -> bool {
    // only switch to combo if the gamemode is active
    if (GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG))
        return false;

    // only if the gamemode wants it
    if (GameModeManager::instance()->isActive()) {  // only switch to combo if any gamemode is active
        return !al::isPadHoldL(-1) && al::isPadTriggerDown(-1);
    } else {
        return al::isPadTriggerDown(-1);
    }
});

static HkTrampoline<void, GameConfigData*, al::ByamlWriter*> saveWriteHook = hk::hook::trampoline([](GameConfigData* cfgData, al::ByamlWriter* writer) -> void {
    saveWriteHook.orig(cfgData, writer);

    const char* serverIP = Client::getCurrentIP();
    const int serverPort = Client::getCurrentPort();
    const bool serverHidden = Client::isServerHidden();

    writer->pushHash("SMOOData");
    if (serverIP) {
        writer->addString("ServerIP", serverIP);
    } else {
        writer->addString("ServerIP", "127.0.0.1");
    }

    if (serverPort) {
        writer->addInt("ServerPort", serverPort);
    } else {
        writer->addInt("ServerPort", 0);
    }

    writer->addBool("ServerHidden", serverHidden);
    writer->pop();
});

static HkTrampoline<void, GameConfigData*, const al::ByamlIter&> saveReadHook =
    hk::hook::trampoline([](GameConfigData* cfgData, const al::ByamlIter& iter) -> void {
        saveReadHook.orig(cfgData, iter);

        const char* serverIP = "";
        int serverPort = 0;
        bool serverHidden = false;

        al::ByamlIter iterIntern;
        al::tryGetByamlIterByKey(&iterIntern, iter, "SMOOData");

        if (al::tryGetByamlString(&serverIP, iterIntern, "ServerIP")) {
            Client::setLastUsedIP(serverIP);
        }

        if (al::tryGetByamlS32(&serverPort, iterIntern, "ServerPort")) {
            Client::setLastUsedPort(serverPort);
        }

        if (al::tryGetByamlBool(&serverHidden, iterIntern, "ServerHidden")) {
            Client::setServerHidden(serverHidden);
        }
    });

static HkTrampoline<void, Shine*> registerShineToListHook = hk::hook::trampoline([](Shine* shine) -> void {
    registerShineToListHook.orig(shine);
    if (shine->mShineIdx >= 0) {
        Client::tryRegisterShine(shine);
    }
});

// void overrideNerveHook(StageSceneStatePauseMenu* thisPtr, al::Nerve* nrvSet) {

//     if (al::isPadHoldZL(-1)) {
//         al::setNerve(thisPtr, &nrvStageSceneStatePauseMenuServerConfig);
//     } else {
//         al::setNerve(thisPtr, nrvSet);
//     }
// }

static HkReplace<void, StageSceneStatePauseMenu*> overrideHelpFadeNerve = hk::hook::replace([](StageSceneStatePauseMenu* state) -> void {
    // Set label in menu inside LocalizedData/${lang}/MessageData/LayoutMessage.szs/Menu.msbt/Menu_Help
    state->exeServerConfig();
    al::setNerve(state, &NrvStageSceneStatePauseMenu.ServerConfig);
});

static StageSceneStateServerConfig* sceneStateServerConfig = nullptr;

static HkTrampoline<void, StageSceneStateOption*, const char*, al::Scene*, const al::LayoutInitInfo&, FooterParts*, GameDataHolder*, bool> initStateHook =
    hk::hook::trampoline([](StageSceneStateOption* thisPtr, const char* stateName, al::Scene* host, const al::LayoutInitInfo& initInfo, FooterParts* footer,
                            GameDataHolder* data, bool unkBool) -> void {
        initStateHook.orig(thisPtr, stateName, host, initInfo, footer, data, unkBool);
        sceneStateServerConfig = new StageSceneStateServerConfig("ServerConfig", host, initInfo, footer, data, unkBool);
    });

static HkTrampoline<void, StageSceneStatePauseMenu*, const char*, al::Scene*, al::SimpleLayoutAppearWaitEnd*, GameDataHolder*, const al::SceneInitInfo&,
                    const al::ActorInitInfo&, const al::LayoutInitInfo&, al::WindowConfirm*, StageSceneLayout*, bool, SceneAudioSystemPauseController*>
    initNerveStateHook = hk::hook::trampoline([](StageSceneStatePauseMenu* state, const char* name, al::Scene* host, al::SimpleLayoutAppearWaitEnd* menuLayout,
                                                 GameDataHolder* gameDataHolder, const al::SceneInitInfo& sceneInitInfo, const al::ActorInitInfo& actorInitInfo,
                                                 const al::LayoutInitInfo& layoutInitInfo, al::WindowConfirm* windowConfirm, StageSceneLayout* stageSceneLayout,
                                                 bool isTitle, SceneAudioSystemPauseController* sceneAudioSystemPauseController) -> void {
        initNerveStateHook.orig(state, name, host, menuLayout, gameDataHolder, sceneInitInfo, actorInitInfo, layoutInitInfo, windowConfirm, stageSceneLayout,
                                isTitle, sceneAudioSystemPauseController);

        al::initNerveState(state, sceneStateServerConfig, &NrvStageSceneStatePauseMenu.ServerConfig, "CustomNerveOverride");
    });

// skips starting both coin counters
static HkTrampoline<void, CoinCounter*> startCoinCounterHook = hk::hook::trampoline([](CoinCounter* counter) -> void {
    if (!GameModeManager::instance()->isModeRequireUI()) {
        startCoinCounterHook.orig(counter);
    }
});

// Simple hook that can be used to override isModeE3 checks to enable/disable certain behaviors
bool modeE3Hook() {
    return GameModeManager::instance()->isModeRequireUI();
}

// Gravity Hooks

void initHackCapHook(al::LiveActor* cappy) {
    al::initActorPoseTQGSV(cappy);
}

// Skips ending the play guide layout if a mode is active, since the mode would have already ended
// it
void playGuideEndHook(al::SimpleLayoutAppearWaitEnd* thisPtr) {
    if (!GameModeManager::instance()->isModeRequireUI()) {
        thisPtr->end();
    }
}

static HkTrampoline<void, StageScene*, al::SceneInitInfo*> stageSceneInitHook =
    hk::hook::trampoline([](StageScene* curScene, al::SceneInitInfo* initInfo) -> void {
        stageSceneInitHook.orig(curScene, initInfo);
        if (GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK)) {
            al::CameraDirector* director = curScene->getCameraDirector();
            if (director) {
                if (director->mPoserFactory) {
                    al::CameraTicket* gravityCamera = director->createCameraFromFactory("CameraPoserCustom", nullptr, 0, 5, sead::Matrix34f::ident);

                    HideAndSeekMode* mode = GameModeManager::instance()->getMode<HideAndSeekMode>();

                    mode->setCameraTicket(gravityCamera);
                }
            }
        }

        if (GameModeManager::instance()->isMode(GameMode::FREEZETAG) || GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK)) {
            al::CameraDirector* director = curScene->getCameraDirector();
            if (director && director->mPoserFactory) {
                al::CameraTicket* spectateCamera = director->createCameraFromFactory("CameraPoserActorSpectate", nullptr, 0, 5, sead::Matrix34f::ident);

                if (GameModeManager::instance()->isMode(GameMode::FREEZETAG)) {
                    FreezeTagMode* mode = GameModeManager::instance()->getMode<FreezeTagMode>();
                    mode->setCameraTicket(spectateCamera);
                } else if (GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK)) {
                    HideAndSeekMode* mode = GameModeManager::instance()->getMode<HideAndSeekMode>();
                    mode->setCameraTicket(spectateCamera);
                }
            }
        }
    });

static HkTrampoline<void, WorldEndBorderKeeper*> borderPullBackHook = hk::hook::trampoline([](WorldEndBorderKeeper* keeper) -> void {
    if (al::isFirstStep(keeper) && GameModeManager::instance()->isActive()) {
        if (GameModeManager::instance()->isModeAndActive(GameMode::HIDEANDSEEK)) {
            HideAndSeekMode* mode = GameModeManager::instance()->getMode<HideAndSeekMode>();

            if (mode->isUseGravity()) {
                killMainPlayer(keeper->mActor);
            }
        }
    }
    borderPullBackHook.orig(keeper);
});

constexpr static al::ExecuteTable DrawTableCustom[] = {
    createDrawTable("OnlineDrawExecutors", "PuppetActor", "ActorModelDrawDeferred", "PuppetActor", "ActorModelDrawDeferred")};

constexpr static al::ExecuteTable UpdateTableCustom[] = {
    createUpdateTable("OnlineUpdateExecutors", "PuppetActor", "PuppetActor"),
};

static HkTrampoline<void, al::ExecuteDirector*, const al::ExecuteSystemInitInfo&> drawTableHook =
    hk::hook::trampoline([](al::ExecuteDirector* director, const al::ExecuteSystemInitInfo& initInfo) -> void {
        drawTableHook.orig(director, initInfo);

        constexpr s32 UpdateTableSize = sizeof(UpdateTableCustom) / sizeof(UpdateTableCustom[0]);
        al::ExecuteTableHolderUpdate** updateTables = new al::ExecuteTableHolderUpdate*[director->mUpdateTableCount + UpdateTableSize]();

        for (s32 i = 0; i < director->mUpdateTableCount; i++) {
            updateTables[i] = director->mUpdateTables[i];
        }
        for (s32 i = 0; i < UpdateTableSize; i++) {
            updateTables[director->mUpdateTableCount + i] = new al::ExecuteTableHolderUpdate();
            const al::ExecuteTable& curTable = UpdateTableCustom[i];
            updateTables[director->mUpdateTableCount + i]->init(curTable.name, initInfo, curTable.executeOrders, curTable.executeOrderCount);
        }
        director->mUpdateTableCount += UpdateTableSize;
        director->mUpdateTables = updateTables;

        constexpr s32 DrawTableSize = sizeof(DrawTableCustom) / sizeof(DrawTableCustom[0]);
        al::ExecuteTableHolderDraw** drawTables = new al::ExecuteTableHolderDraw*[director->mDrawTableCount + DrawTableSize]();
        for (s32 i = 0; i < director->mDrawTableCount; i++) {
            drawTables[i] = director->mDrawTables[i];
        }
        for (s32 i = 0; i < DrawTableSize; i++) {
            drawTables[director->mDrawTableCount + i] = new al::ExecuteTableHolderDraw();
            const al::ExecuteTable& curTable = DrawTableCustom[i];
            drawTables[director->mDrawTableCount + i]->init(curTable.name, initInfo, curTable.executeOrders, curTable.executeOrderCount);
        }
        director->mDrawTableCount += DrawTableSize;
        director->mDrawTables = drawTables;
    });

static HkTrampoline<bool, al::IUseStageSwitch*, const char*, const al::FunctorBase&> unlockCostumeDoorsHook =
    hk::hook::trampoline([](al::IUseStageSwitch* user, const char* eventName, const al::FunctorBase& action) -> bool {
        if (strcmp(eventName, "OpenKeySwitch") == 0 && StageSceneStateServerConfig::isCostumeDoorsUnlocked())
            return false;
        return unlockCostumeDoorsHook.orig(user, eventName, action);
    });

static bool unlockCostumeDoorMetroHook(const char* str1, const char* str2) {
    if (StageSceneStateServerConfig::isCostumeDoorsUnlocked())
        return true;
    return al::isEqualString(str1, str2);
}

static HkTrampoline<bool, al::Triangle&, char*> icePhysicsHook = hk::hook::trampoline([](al::Triangle& triangle, char* floorCode) -> bool {
    if (strcmp(floorCode, "Skate") != 0)
        return icePhysicsHook.orig(triangle, floorCode);

    bool isNaturalIce = icePhysicsHook.orig(triangle, floorCode);

    return isNaturalIce ? true : TwistsConfig::isIcePhysicsEnabled();
});

static HkTrampoline<void, StageSceneStatePauseMenu*> menuTextHook = hk::hook::trampoline([](StageSceneStatePauseMenu* menu) -> void {
    if (al::isFirstStep(menu))
        menu->mSelectParts->setSelectMessage(2, u"Mod Menu");

    menuTextHook.orig(menu);
});

static HkTrampoline<void, AppearSwitchTimer*, const al::ActorInitInfo&, const al::IUseAudioKeeper*, al::IUseStageSwitch*, al::IUseCamera*, al::LiveActor*>
    disableAppearSwitchCameraHook = hk::hook::trampoline([](AppearSwitchTimer* timer, const al::ActorInitInfo& initInofo, const al::IUseAudioKeeper* audio,
                                                            al::IUseStageSwitch* stageSwitch, al::IUseCamera* camera, al::LiveActor* actor) -> void {
        disableAppearSwitchCameraHook.orig(timer, initInofo, audio, stageSwitch, camera, actor);
        timer->mDemoCameraFrame = 0;
    });

static HkTrampoline<bool, al::WindowConfirmWait*> windowConfirmWaitHook = hk::hook::trampoline([](al::WindowConfirmWait* win) -> bool {
    al::setNerve(win, (al::Nerve*)(hk::ro::getMainModule()->range().start() + 0x1e05be8));
    return true;
});

static HkTrampoline<void, StageSceneStatePauseMenu*> pauseMenuAppearHook = hk::hook::trampoline([](StageSceneStatePauseMenu* menu) -> void {
    if (al::isFirstStep(menu)) {
        ConnectionStatus::sInstance->tryStart();
    }

    pauseMenuAppearHook.orig(menu);
});
static HkTrampoline<void, StageSceneStatePauseMenu*> pauseMenuEndHook = hk::hook::trampoline([](StageSceneStatePauseMenu* menu) -> void {
    if (al::isFirstStep(menu)) {
        ConnectionStatus::sInstance->tryEnd();
    }
    pauseMenuEndHook.orig(menu);
});