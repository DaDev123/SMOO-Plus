#include "hk/gfx/ImGuiBackendNvn.h"
#include "hk/hook/Trampoline.h"

#include "al/Library/Bgm/BgmLineFunction.h"
#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Controller/PadRumbleDirector.h"
#include "al/Library/Controller/PadRumbleFunction.h"
#include "al/Library/Player/PlayerUtil.h"
#include "al/Library/Scene/SceneUtil.h"
#include "al/Library/System/GameSystemInfo.h"

#include "agl/common/aglDrawContext.h"

#include "game/MapObj/CheckpointFlagWatcher.h"
#include "game/System/Application.h"
#include "game/System/GameConfigData.h"
#include "game/System/GameDataHolderAccessor.h"
#include "game/System/GameSystem.h"

#include "imgui.h"
#include "layouts/PlayerEventLog.h"
#include "layouts/SpeedrunIcon.h"
#include "main.hpp"
#include "saveManager.h"
#include "Scene/StageSceneStateModConfig.hpp"
#include "server/Client.hpp"
#include "server/DeltaTime.hpp"

HkTrampoline saveWriteHook = [](TrampolineStatic(), GameConfigData* cfgData,
                                al::ByamlWriter* origWriter) -> void {
    Client::instance()->saveMoonRocks(origWriter);

    orig(cfgData, origWriter);

    SaveManager::instance()->startThread(cfgData);
};

HkTrampoline saveReadHook = [](TrampolineStatic(), GameConfigData* cfgData,
                               const al::ByamlIter& origIter) -> void {
    Client::instance()->readMoonRocks(origIter);

    orig(cfgData, origIter);

    SaveManager::instance()->read(cfgData);
};

HkTrampoline drawMainHookHk = [](TrampolineStatic(), GameSystem* gameSystem) -> void {
    orig(gameSystem);

    auto* drawContext = Application::instance()->mDrawSystemInfo->drawContext;

    /* ImGui */

    ImGui::NewFrame();
    ImGui::GetIO().DeltaTime = Time::deltaTime;
    drawMain(gameSystem->mSequence);

    ImGui::Render();

    hk::gfx::ImGuiBackendNvn::instance()->draw(ImGui::GetDrawData(),
                                               drawContext->getCommandBuffer()->ToData()->pNvnCommandBuffer);
};

HkTrampoline sceneKillHook = [](TrampolineStatic(), StageScene* scene) -> void {
    // this hook should prevent crashes on scene transitions
    gIsSceneAlive = false;

    Client::clearArrays();

    orig(scene);
};

HkTrampoline hakoniwaSequenceHook = [](TrampolineStatic(), HakoniwaSequence* sequence) -> void {
    StageScene* stageScene = (StageScene*)sequence->mCurrentScene;
    SpeedrunIcon::sInstance->setHolder(sequence->mGameDataHolderAccessor);

    static bool isCameraActive = false;

    al::PlayerHolder* pHolder = al::getScenePlayerHolder(stageScene);
    PlayerActorBase* playerBase = (PlayerActorBase*)al::tryGetPlayerActor(pHolder, 0);
    auto* player = (PlayerActorHakoniwa*)al::tryGetPlayerActor(pHolder, 0);

    if (!playerBase) {
        orig(sequence);
        return;
    }

    bool isYukimaru = !playerBase->getPlayerInfo();

    isInGame = !stageScene->isPause();

    Client::setStageInfo(sequence);

    Client::update();

    if (Client::shouldStopRumble() && player && !isYukimaru) {
        auto* rumbleDirector = alPadRumbleFunction::getPadRumbleDirector(player);
        if (rumbleDirector)
            rumbleDirector->stopAllRumble();
        Client::clearStopRumble();
    }

    if (gIsSceneAlive) {
        updatePlayerInfo(GameDataHolderAccessor(stageScene), playerBase, isYukimaru);
    }

    if (SpeedrunIcon::sInstance) {
        if (StageSceneStateModConfig::isSpeedrunModeEnabled()) {
            SpeedrunIcon::sInstance->tryStart();

        } else {
            SpeedrunIcon::sInstance->tryEnd();
        }
    }

    stageScene->stageSceneLayout->updateCounterParts();

    if (al::isPadHoldZR(-1)) {
        if (al::isPadTriggerUp(-1)) {  // ZR + Up => Debug menu
            debugMode = !debugMode;
        }
        if (debugMode) {
            if (al::isPadTriggerLeft(-1)) {  // [Debug menu] ZR + Left => Previous page
                pageIndex--;
                if (pageIndex < 0) {
                    pageIndex = maxPages - 1;
                }
            }
            if (al::isPadTriggerRight(-1)) {  // [Debug menu] ZR + Right => Next page
                pageIndex++;
                if (pageIndex >= maxPages) {
                    pageIndex = 0;
                }
            }
        }
    } else if (al::isPadHoldZL(-1)) {
        if (debugMode && pageIndex == 0) {
            if (al::isPadTriggerLeft(-1)) {  // [Debug menu] ZL + Left => Previous player
                debugPuppetIndex--;
                if (debugPuppetIndex < 0) {
                    debugPuppetIndex = Client::getMaxPlayerCount() - 1;
                }
            }
            if (al::isPadTriggerRight(-1)) {  // [Debug menu] ZL + Right => Next player
                debugPuppetIndex++;
                if (debugPuppetIndex >= Client::getMaxPlayerCount()) {
                    debugPuppetIndex = 0;
                }
            }
        }
        if (al::isPadTriggerUp(-1)) {
            if (PlayerEventLog::sInstance)
                PlayerEventLog::toggleShow();
        }
    } else if (al::isPadHoldL()) {
        if (al::isPadTriggerUp()) {
            Client::sInstance->setStopRumble();
        }
    }
    if (Client::isMusicDisabled()) {
        if (al::isPlayingBgm(stageScene)) {
            al::stopAllBgm(stageScene, 0);
        }
    }

    orig(sequence);
};

// ===== LOGGING HOOK =====

void seadPrintHook(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    hk::diag::log(fmt, args);
    va_end(args);
}

HkReplaceVarArgs replaceSeadPrintHook = seadPrintHook;

// crash fix maybe
HkTrampoline checkpointFlagWatcherHook = [](TrampolineStatic(), CheckpointFlagWatcher* thisPtr,
                                            char* p1) -> void* {
    if (!thisPtr || !p1)
        return nullptr;

    return orig(thisPtr, p1);
};

void installOtherHooks() {
    replaceSeadPrintHook.installAtSym<"_ZN4sead6system5PrintEPKcz">();
    drawMainHookHk.installAtSym<"_ZN10GameSystem8drawMainEv">();
    // Main Stuff
    hakoniwaSequenceHook.installAtSym<"_ZN16HakoniwaSequence12exePlayStageEv">();

    // Save Data Edits
    saveWriteHook.installAtSym<"_ZN14GameConfigData5writeEPN2al11ByamlWriterE">();
    saveReadHook.installAtSym<"_ZN14GameConfigData4readERKN2al9ByamlIterE">();

    sceneKillHook.installAtSym<"_ZN10StageScene4killEv">();

    checkpointFlagWatcherHook.installAtSym<"_ZNK21CheckpointFlagWatcher21tryFindCheckpointFlagEPKc">();
}