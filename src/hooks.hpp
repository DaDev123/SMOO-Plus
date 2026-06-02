#include "hk/hook/a64/Assembler.h"
#include "hk/hook/Replace.h"
#include "hk/hook/Trampoline.h"
#include "hk/util/Math.h"

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

#include "game/Item/Shine.h"
#include "game/Layout/CoinCounter.h"
#include "game/MapObj/AppearSwitchTimer.h"
#include "game/Scene/StageSceneStateOption.h"
#include "game/Scene/StageSceneStatePauseMenu.h"

#include <cstring>
#include <sys/types.h>

#include "CheckpointMasterList.h"
#include "Imgui.hpp"
#include "Item/CoinCollect.h"
#include "layouts/ConnectionStatus.h"
#include "Library/Collision/CollisionPartsTriangle.h"
#include "Library/Nerve/Nerve.h"
#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.h"
#include "logger.hpp"
#include "main.hpp"
#include "Scene/StageScene.h"
#include "Scene/StageSceneStateModConfig.hpp"
#include "Sequence/HakoniwaSequence.h"
#include "server/Client.hpp"
#include "System/GameDataFile.h"
#include "System/GameDataFunction.h"
#include "System/GameDataHolder.h"
#include "System/GameDataHolderAccessor.h"
#include "System/UniqObjInfo.h"

static HkTrampoline saveWriteHook = [](TrampolineStatic(), GameConfigData* cfgData,
                                       al::ByamlWriter* writer) -> void {
    orig(cfgData, writer);

    const char* serverIP = Client::getCurrentIP();
    const int serverPort = Client::getCurrentPort();
    const bool serverHidden = Client::isServerHidden();
    const bool capCollision = StageSceneStateModConfig::isCapCollisionEnabled();
    const bool capBounce = StageSceneStateModConfig::isCapBounceEnabled();
    const bool playerCollision = StageSceneStateModConfig::isPuppetCollisionEnabled();
    const bool playerBounce = StageSceneStateModConfig::isPuppetBounceEnabled();
    const bool costumeDoorsUnlocked = StageSceneStateModConfig::isCostumeDoorsUnlocked();
    const bool lowLatency = StageSceneStateModConfig::isLowLatencyEnabled();
    const bool music = !Client::isMusicDisabled();

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
    writer->addBool("CapCollision", capCollision);
    writer->addBool("CapBounce", capBounce);
    writer->addBool("PlayerCollision", playerCollision);
    writer->addBool("PlayerBounce", playerBounce);
    writer->addBool("CostumeDoorsUnlocked", costumeDoorsUnlocked);
    writer->addBool("LowLatency", lowLatency);
    writer->addBool("Music", music);
    writer->pop();
};

static HkTrampoline saveReadHook = [](TrampolineStatic(), GameConfigData* cfgData,
                                      const al::ByamlIter& iter) -> void {
    orig(cfgData, iter);

    const char* serverIP = "";
    int serverPort = 0;
    bool serverHidden = false;
    bool capCollision = false;
    bool capBounce = false;
    bool playerCollision = true;
    bool playerBounce = true;
    bool costumeDoorsUnlocked = true;
    bool lowLatency = false;
    bool music = true;

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

    if (al::tryGetByamlBool(&capCollision, iterIntern, "CapCollision")) {
        StageSceneStateModConfig::setCapCollisionEnabled(capCollision);
    }
    if (al::tryGetByamlBool(&capBounce, iterIntern, "CapBounce")) {
        StageSceneStateModConfig::setCapBounceEnabled(capBounce);
    }
    if (al::tryGetByamlBool(&playerCollision, iterIntern, "PlayerCollision")) {
        StageSceneStateModConfig::setPuppetCollisionEnabled(playerCollision);
    }
    if (al::tryGetByamlBool(&playerBounce, iterIntern, "PlayerBounce")) {
        StageSceneStateModConfig::setPuppetBounceEnabled(playerBounce);
    }
    if (al::tryGetByamlBool(&costumeDoorsUnlocked, iterIntern, "CostumeDoorsUnlocked")) {
        StageSceneStateModConfig::setCostumeDoorsUnlocked(costumeDoorsUnlocked);
    }
    if (al::tryGetByamlBool(&lowLatency, iterIntern, "LowLatency")) {
        StageSceneStateModConfig::setLowLatencyEnabled(lowLatency);
    }
    if (al::tryGetByamlBool(&music, iterIntern, "Music")) {
        if (Client::isMusicDisabled() != !music) {
            Client::toggleMusicDisabled();
        }
    }
};

static HkTrampoline registerShineToListHook = [](TrampolineStatic(), Shine* shine) -> void {
    orig(shine);
    if (shine->mShineIdx >= 0) {
        Client::tryRegisterShine(shine);
    }
};
class CoinCollectHolder;
static HkTrampoline registerCoinCollectToListHook = [](TrampolineStatic(), CoinCollectHolder* h,
                                                       CoinCollect* coin) -> void {
    orig(h, coin);
    Client::tryRegisterCoinCollect(coin);
};

static HkTrampoline registerCoinCollect2DToListHook = [](TrampolineStatic(), CoinCollectHolder* h,
                                                         CoinCollect2D* coin) -> void {
    orig(h, coin);
    Client::tryRegisterCoinCollect2D(coin);
};

static HkReplace<bool, GameDataFile*, s32> isGotCheckpointInWorldHook =
    hk::hook::replace([](GameDataFile* gdf, s32 index) -> bool {
        s32 index2 = gdf->calcCheckpointIndexInScenario(index);
        if (index2 < 0)
            return false;
        const char* checkpointName =
            gdf->getCheckpointTable()[gdf->getCurrentWorldIdNoDevelop()][index2].objInfo.getObjId();
        for (s32 i = 0; i < CheckpointMasterList::sNumCheckpoints; i++) {
            UniqObjInfo info = gdf->getGotCheckpointTable()[i];
            if (al::isEqualString(checkpointName, info.getObjId())) {
                return true;
            }
        }
        return false;
    });

static HkReplace<void, StageSceneStatePauseMenu*> overrideHelpFadeNerve =
    hk::hook::replace([](StageSceneStatePauseMenu* state) -> void {
        // Set label in menu inside LocalizedData/${lang}/MessageData/LayoutMessage.szs/Menu.msbt/Menu_Help
        state->exeModConfig();
        al::setNerve(state, &NrvStageSceneStatePauseMenu.ModConfig);
    });

static StageSceneStateModConfig* sceneStateModConfig = nullptr;

static HkTrampoline initStateHook =
    [](TrampolineStatic(), StageSceneStateOption* thisPtr, const char* stateName, al::Scene* host,
       const al::LayoutInitInfo& initInfo, FooterParts* footer, GameDataHolder* data, bool unkBool) -> void {
    orig(thisPtr, stateName, host, initInfo, footer, data, unkBool);
    sceneStateModConfig = new StageSceneStateModConfig("ModConfig", host, initInfo, footer, data, unkBool);
};

static HkTrampoline initNerveStateHook =
    [](TrampolineStatic(), StageSceneStatePauseMenu* state, const char* name, al::Scene* host,
       al::SimpleLayoutAppearWaitEnd* menuLayout, GameDataHolder* gameDataHolder,
       const al::SceneInitInfo& sceneInitInfo, const al::ActorInitInfo& actorInitInfo,
       const al::LayoutInitInfo& layoutInitInfo, al::WindowConfirm* windowConfirm,
       StageSceneLayout* stageSceneLayout, bool isTitle,
       SceneAudioSystemPauseController* sceneAudioSystemPauseController) -> void {
    orig(state, name, host, menuLayout, gameDataHolder, sceneInitInfo, actorInitInfo, layoutInitInfo,
         windowConfirm, stageSceneLayout, isTitle, sceneAudioSystemPauseController);

    al::initNerveState(state, sceneStateModConfig, &NrvStageSceneStatePauseMenu.ModConfig,
                       "CustomNerveOverride");
};

constexpr static al::ExecuteTable DrawTableCustom[] = {createDrawTable(
    "OnlineDrawExecutors", "PuppetActor", "ActorModelDrawDeferred", "PuppetActor", "ActorModelDrawDeferred")};

constexpr static al::ExecuteTable UpdateTableCustom[] = {
    createUpdateTable("OnlineUpdateExecutors", "PuppetActor", "PuppetActor"),
};

static HkTrampoline drawTableHook = [](TrampolineStatic(), al::ExecuteDirector* director,
                                       const al::ExecuteSystemInitInfo& initInfo) -> void {
    orig(director, initInfo);

    constexpr s32 UpdateTableSize = sizeof(UpdateTableCustom) / sizeof(UpdateTableCustom[0]);
    al::ExecuteTableHolderUpdate** updateTables =
        new al::ExecuteTableHolderUpdate*[director->mUpdateTableCount + UpdateTableSize]();

    for (s32 i = 0; i < director->mUpdateTableCount; i++) {
        updateTables[i] = director->mUpdateTables[i];
    }
    for (s32 i = 0; i < UpdateTableSize; i++) {
        updateTables[director->mUpdateTableCount + i] = new al::ExecuteTableHolderUpdate();
        const al::ExecuteTable& curTable = UpdateTableCustom[i];
        updateTables[director->mUpdateTableCount + i]->init(curTable.name, initInfo, curTable.executeOrders,
                                                            curTable.executeOrderCount);
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
        drawTables[director->mDrawTableCount + i]->init(curTable.name, initInfo, curTable.executeOrders,
                                                        curTable.executeOrderCount);
    }
    director->mDrawTableCount += DrawTableSize;
    director->mDrawTables = drawTables;
};

static HkTrampoline unlockCostumeDoorsHook = [](TrampolineStatic(), al::IUseStageSwitch* user,
                                                const char* eventName,
                                                const al::FunctorBase& action) -> bool {
    if (strcmp(eventName, "OpenKeySwitch") == 0 && StageSceneStateModConfig::isCostumeDoorsUnlocked())
        return false;
    return orig(user, eventName, action);
};

static bool unlockCostumeDoorMetroHook(const char* str1, const char* str2) {
    if (StageSceneStateModConfig::isCostumeDoorsUnlocked())
        return true;
    return al::isEqualString(str1, str2);
}

static HkTrampoline pauseMenuWaitHook = [](TrampolineStatic(), StageSceneStatePauseMenu* menu) -> void {
    if (al::isFirstStep(menu))
        menu->mSelectParts->setSelectMessage(2, u"Mod Menu");

    orig(menu);

    if (!menu->isDrawLayout()) {
        ConnectionStatus::sInstance->tryStart();
    } else {
        ConnectionStatus::sInstance->tryEnd();
    }
};

static HkTrampoline disableAppearSwitchCameraHook =
    [](TrampolineStatic(), AppearSwitchTimer* timer, const al::ActorInitInfo& initInofo,
       const al::IUseAudioKeeper* audio, al::IUseStageSwitch* stageSwitch, al::IUseCamera* camera,
       al::LiveActor* actor) -> void {
    orig(timer, initInofo, audio, stageSwitch, camera, actor);
    timer->mDemoCameraFrame = 0;
};

static HkTrampoline windowConfirmWaitHook = [](TrampolineStatic(), al::WindowConfirmWait* win) -> bool {
    al::setNerve(win, (al::Nerve*)(hk::ro::getMainModule()->range().start() + 0x1e05be8));
    return true;
};

static HkTrampoline resetScenarioSyncHook = [](TrampolineStatic(), HakoniwaSequence* seq) -> void {
    orig(seq);
    GameDataFile::FixedHeapArray<s32, sNumWorlds> scenNumArr =
        Client::sInstance->getHolder()->getGameDataFile()->getScenarioNumArr();
    GameDataFile::FixedHeapArray<s32, sNumWorlds> mainSenNumArr =
        Client::sInstance->getHolder()->getGameDataFile()->getMainScenarioNumArr();

    Logger::log("Resetting Scenarios\n");
    for (int i = 0; i < sNumWorlds; i++) {
        Logger::log("%d: Scen: %d, MainScen: %d\n", i, scenNumArr[i], mainSenNumArr[i]);
        // scenNumArr[i] = 1;
        // mainSenNumArr[i] = -1;
        // Logger::log("%d: Scen: %d, MainScen: %d\n", i, scenNumArr[i], mainSenNumArr[i]);
    }
    shouldResetScenario = true;
};

namespace speedrun {

static bool isHooksCreated = false;

static constexpr u32 listPtrNop[] = {
    0x4DB934,
    0x2D250C,
};

static hk::hook::a64::AsmBlock<true, 1>* listNop[hk::util::arraySize(listPtrNop)];

static void uninstallHooks() {
    for (int i = 0; i < hk::util::arraySize(listPtrNop); i++) {
        listNop[i]->uninstall();
    }
}

static void createHooks() {
    if (!isHooksCreated) {
        for (int i = 0; i < hk::util::arraySize(listPtrNop); i++) {
            listNop[i] = new (imgui::sImGuiHeap)
                hk::hook::a64::AsmBlock<true, 1>(hk::hook::a64::assemble<"nop", true>());
        }
        isHooksCreated = true;
    }
}

static void installHooks() {
    for (int i = 0; i < hk::util::arraySize(listPtrNop); i++) {
        listNop[i]->installAtMainOffset(listPtrNop[i]);
    }
}

}  // namespace speedrun