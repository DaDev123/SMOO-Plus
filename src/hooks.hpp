#include "hk/hook/Replace.h"
#include "hk/hook/Trampoline.h"

#include "nn/fs/fs_mount.h"

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
#include "al/Library/Yaml/Writer/ByamlWriter.h"

#include "game/Item/Shine.h"
#include "game/Layout/CoinCounter.h"
#include "game/MapObj/AppearSwitchTimer.h"
#include "game/Scene/StageSceneStateOption.h"
#include "game/Scene/StageSceneStatePauseMenu.h"
#include "game/System/GameConfigData.h"

#include <cstring>
#include <sys/types.h>

#include "filedevice/nin/seadNinFileDeviceBaseNin.h"
#include "filedevice/seadFileDeviceMgr.h"
#include "helpers.hpp"
#include "Item/CoinCollect.h"
#include "layouts/ConnectionStatus.h"
#include "layouts/PlayerEventLog.h"
#include "Library/Collision/CollisionPartsTriangle.h"
#include "Library/Memory/HeapUtil.h"
#include "Library/Nerve/Nerve.h"
#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.h"
#include "Library/Thread/FunctorV0M.h"
#include "main.hpp"
#include "MapObj/ChangeStageInfo.h"
#include "MapObj/MoonRock.h"
#include "saveManager.h"
#include "Scene/StageScene.h"
#include "Scene/StageSceneStateModConfig.hpp"
#include "Sequence/HakoniwaSequence.h"
#include "server/Client.hpp"
#include "System/GameDataFile.h"
#include "System/GameDataFunction.h"
#include "System/GameDataHolder.h"
#include "System/GameDataHolderAccessor.h"

static HkTrampoline saveWriteHook = [](TrampolineStatic(), GameConfigData* cfgData,
                                       al::ByamlWriter* origWriter) -> void {
    Client::instance()->saveMoonRocks(origWriter);

    orig(cfgData, origWriter);

    SaveManager::instance()->startThread(cfgData);
};

static HkTrampoline saveReadHook = [](TrampolineStatic(), GameConfigData* cfgData,
                                      const al::ByamlIter& origIter) -> void {
    Client::instance()->readMoonRocks(origIter);

    orig(cfgData, origIter);

    SaveManager::instance()->read(cfgData);
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
    sceneStateModConfig =
        new (al::getSceneHeap()) StageSceneStateModConfig("ModConfig", host, initInfo, footer, data, unkBool);
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

static HkTrampoline pauseMenuAppearHook = [](TrampolineStatic(), StageSceneStatePauseMenu* menu) -> void {
    if (al::isFirstStep(menu))
        menu->mSelectParts->setSelectMessage(2, u"Mod Menu");

    orig(menu);
};
static HkTrampoline pauseMenuWaitHook = [](TrampolineStatic(), StageSceneStatePauseMenu* menu) -> void {
    orig(menu);

    if (ConnectionStatus::sInstance) {
        if (!menu->isDrawLayout())
            ConnectionStatus::sInstance->tryStart();
        else
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

    PlayerEventLog::addSelfEvent(PlayerEventLog::START, "");

    Client::sendGameInfPacket(seq->mGameDataHolderAccessor, true);
};

static HkTrampoline moonRockHook = [](TrampolineStatic(), MoonRock* moonRock) -> void {
    if (al::isFirstStep(moonRock)) {
        Client::sendMoonRockHitPacket(GameDataFunction::getCurrentWorldIdNoDevelop(moonRock));

        PlayerEventLog::addSelfEvent(PlayerEventLog::MOONROCK,
                                     worldNames[GameDataFunction::getCurrentWorldIdNoDevelop(moonRock)]);
    }

    orig(moonRock);
};

static HkTrampoline mountSdCardHook = [](TrampolineStatic(), sead::FileDeviceMgr* fileDeviceMgr) -> void {
    orig(fileDeviceMgr);

    fileDeviceMgr->mMountedSd = nn::fs::MountSdCardForDebug("sd") == 0;
    sead::NinFileDeviceBase* sdFileDevice = new (gHeap) sead::NinFileDeviceBase("sd", "sd");
    fileDeviceMgr->mount(sdFileDevice);
};
