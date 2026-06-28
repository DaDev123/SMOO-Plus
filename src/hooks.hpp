#include "hk/hook/a64/Assembler.h"
#include "hk/hook/Replace.h"
#include "hk/hook/Trampoline.h"

#include "nn/fs/fs_directories.h"
#include "nn/fs/fs_mount.h"

#include "sead/heap/seadHakkunHeap.h"

#include "al/Library/Camera/CameraDirector.h"
#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Execute/ExecuteDirector.h"
#include "al/Library/Execute/ExecuteRequestKeeper.h"
#include "al/Library/Execute/ExecuteTableHolderDraw.h"
#include "al/Library/Execute/ExecuteTableHolderUpdate.h"
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
#include "game/System/GameConfigData.h"

#include <cstring>
#include <sys/types.h>

#include "CheckpointMasterList.h"
#include "filedevice/nin/seadNinFileDeviceBaseNin.h"
#include "filedevice/seadFileDeviceMgr.h"
#include "fsHelper.h"
#include "heap/seadHeapMgr.h"
#include "helpers.hpp"
#include "Item/CoinCollect.h"
#include "layouts/ConnectionStatus.h"
#include "layouts/PlayerEventLog.h"
#include "Library/Collision/CollisionPartsTriangle.h"
#include "Library/Nerve/Nerve.h"
#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.h"
#include "Library/Thread/AsyncFunctorThread.h"
#include "Library/Thread/FunctorV0M.h"
#include "logger.hpp"
#include "MapObj/ChangeStageInfo.h"
#include "MapObj/MoonRock.h"
#include "puppets/PuppetMain.hpp"
#include "Scene/StageScene.h"
#include "Scene/StageSceneStateModConfig.hpp"
#include "Sequence/HakoniwaSequence.h"
#include "server/Client.hpp"
#include "System/GameDataFile.h"
#include "System/GameDataFunction.h"
#include "System/GameDataHolder.h"
#include "System/GameDataHolderAccessor.h"
#include "System/UniqObjInfo.h"

static al::AsyncFunctorThread* saveWriteThread = nullptr;

static HkTrampoline saveWriteHook = [](TrampolineStatic(), GameConfigData* cfgData,
                                       al::ByamlWriter* origWriter) -> void {
    Client::instance()->saveMoonRocks(origWriter);

    orig(cfgData, origWriter);

    sead::ScopedCurrentHeapSetter setter(sead::HakkunHeap::sInstance);

    if (!saveWriteThread)
        saveWriteThread =
            new al::AsyncFunctorThread("SaveWriteThread",
                                       al::FunctorV0M<GameConfigData*, GameConfigData::SaveWriteThreadFunc>(
                                           cfgData, &GameConfigData::writeToSd),
                                       16, 0x1000, {0});
    else
        static_cast<al::FunctorV0M<GameConfigData*, GameConfigData::SaveWriteThreadFunc>*>(
            saveWriteThread->mFunctor)
            ->mObjPointer = cfgData;

    if (saveWriteThread->isDone())
        saveWriteThread->start();
};

static HkTrampoline saveReadHook = [](TrampolineStatic(), GameConfigData* cfgData,
                                      const al::ByamlIter& origIter) -> void {
    Logger::log("save read hook\n");
    logHakkunHeapUsage();

    Client::instance()->readMoonRocks(origIter);

    orig(cfgData, origIter);

    if (!FsHelper::isFileExist(sSettingsPath)) {
        nn::fs::CreateDirectory(sModFolder);
        return;
    }

    const char* serverIP = "";
    s32 serverPort = 0;
    bool serverHidden = false;
    bool capCollision = false;
    bool capBounce = false;
    bool playerCollision = true;
    bool playerBounce = true;
    bool costumeDoorsUnlocked = true;
    bool lowLatency = false;
    s32 logLife = 0;
    bool log = true;
    bool shineCount = true;
    bool music = true;

    FsHelper::LoadData data = {.path = sSettingsPath};
    FsHelper::loadFileFromPath(data);

    al::ByamlIter rootIter((u8*)data.buffer);
    al::ByamlIter smooIter;
    al::ByamlIter gameIter;

    if (al::tryGetByamlIterByKey(&smooIter, rootIter, "SMOOData")) {
        if (al::tryGetByamlString(&serverIP, smooIter, "ServerIP"))
            Client::setLastUsedIP(serverIP);
        if (al::tryGetByamlS32(&serverPort, smooIter, "ServerPort"))
            Client::setLastUsedPort(serverPort);
        if (al::tryGetByamlBool(&serverHidden, smooIter, "ServerHidden"))
            Client::setServerHidden(serverHidden);
        if (al::tryGetByamlBool(&capCollision, smooIter, "CapCollision"))
            StageSceneStateModConfig::setCapCollisionEnabled(capCollision);
        if (al::tryGetByamlBool(&capBounce, smooIter, "CapBounce"))
            StageSceneStateModConfig::setCapBounceEnabled(capBounce);
        if (al::tryGetByamlBool(&playerCollision, smooIter, "PlayerCollision"))
            StageSceneStateModConfig::setPuppetCollisionEnabled(playerCollision);
        if (al::tryGetByamlBool(&playerBounce, smooIter, "PlayerBounce"))
            StageSceneStateModConfig::setPuppetBounceEnabled(playerBounce);
        if (al::tryGetByamlBool(&costumeDoorsUnlocked, smooIter, "CostumeDoorsUnlocked"))
            StageSceneStateModConfig::setCostumeDoorsUnlocked(costumeDoorsUnlocked);
        if (al::tryGetByamlBool(&lowLatency, smooIter, "LowLatency"))
            StageSceneStateModConfig::setLowLatencyEnabled(lowLatency);
        if (al::tryGetByamlS32(&logLife, smooIter, "LogLife"))
            StageSceneStateModConfig::setSpeedrunLogLife((StageSceneStateModConfig::SpeedrunLogLife)logLife);
        if (al::tryGetByamlBool(&log, smooIter, "Log"))
            PlayerEventLog::setShow(log);
        if (al::tryGetByamlBool(&shineCount, smooIter, "ShineCount"))
            StageSceneStateModConfig::setShineCountEnabled(shineCount);
        if (al::tryGetByamlBool(&music, smooIter, "Music")) {
            if (Client::isMusicDisabled() != !music) {
                Client::toggleMusicDisabled();
            }
        }
    }
    if (al::tryGetByamlIterByKey(&gameIter, rootIter, "GameConfigData")) {
        al::tryGetByamlS32(&cfgData->mCameraStickSensitivityLevel, gameIter, "CameraStickSensitivityLevel");
        al::tryGetByamlBool(&cfgData->mIsCameraReverseInputH, gameIter, "IsCameraReverseInputH");
        al::tryGetByamlBool(&cfgData->mIsCameraReverseInputV, gameIter, "IsCameraReverseInputV");
        al::tryGetByamlBool(&cfgData->mIsValidCameraGyro, gameIter, "IsValidCameraGyro");
        al::tryGetByamlS32(&cfgData->mCameraGyroSensitivityLevel, gameIter, "CameraGyroSensitivityLevel");
        al::tryGetByamlBool(&cfgData->mIsUseOpenListAdditionalButton, gameIter,
                            "IsUseOpenListAdditionalButton");
        al::tryGetByamlBool(&cfgData->mIsValidPadRumble, gameIter, "IsPadRumble");
        al::tryGetByamlS32(&cfgData->mPadRumbleLevel, gameIter, "PadRumbleLevel");
    }

    free(data.buffer);

    Logger::log("end save read hook\n");
    logHakkunHeapUsage();
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
    sceneStateModConfig = new (Client::instance()->mHakkunSceneHeap)
        StageSceneStateModConfig("ModConfig", host, initInfo, footer, data, unkBool);
    Logger::log("created new mod menu\n");
    logHakkunHeapUsage();
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

static HkTrampoline startNewGameHook = [](TrampolineStatic(), HakoniwaSequence* seq) -> void {
    orig(seq);

    if (PlayerEventLog::sInstance)
        PlayerEventLog::sInstance->addSelfEvent(PlayerEventLog::START, "");

    Client::sendGameInfPacket(seq->mGameDataHolderAccessor, true);
};

static HkTrampoline moonRockHook = [](TrampolineStatic(), MoonRock* moonRock) -> void {
    if (al::isFirstStep(moonRock)) {
        Client::sendMoonRockHitPacket(GameDataFunction::getCurrentWorldIdNoDevelop(moonRock));

        if (PlayerEventLog::sInstance) {
            PlayerEventLog::sInstance->addSelfEvent(
                PlayerEventLog::MOONROCK, worldNames[GameDataFunction::getCurrentWorldIdNoDevelop(moonRock)]);
        }
    }

    orig(moonRock);
};

static HkTrampoline mountSdCardHook = [](TrampolineStatic(), sead::FileDeviceMgr* fileDeviceMgr) -> void {
    orig(fileDeviceMgr);

    fileDeviceMgr->mMountedSd = nn::fs::MountSdCardForDebug("sd") == 0;
    sead::NinFileDeviceBase* sdFileDevice = new sead::NinFileDeviceBase("sd", "sd");
    fileDeviceMgr->mount(sdFileDevice);
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
            listNop[i] = new hk::hook::a64::AsmBlock<true, 1>(hk::hook::a64::assemble<"nop", true>());
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