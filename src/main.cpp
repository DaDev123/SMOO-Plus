/**
 * @file main.cpp
 * @brief Core game hooks and main functionality for the multiplayer client
 */

#include "main.hpp"

#include "hk/diag/diag.h"
#include "hk/hook/a64/Assembler.h"
#include "hk/hook/InstrUtil.h"
#include "hk/hook/Replace.h"
#include "hk/hook/Trampoline.h"

#include "nn/hid.h"  // IWYU pragma: keep
#include "nn/init.h"
#include "nn/nifm.h"
#include "nn/oe.h"
#include "nn/socket.h"

#include <sead/gfx/seadCamera.h>
#include <sead/gfx/seadPrimitiveRenderer.h>
#include <sead/gfx/seadProjection.h>
#include <sead/heap/seadHeap.h>
#include <sead/prim/seadStringUtil.h>

#include "al/Library/Bgm/BgmLineFunction.h"
#include "al/Library/Camera/CameraUtil.h"
#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Controller/PadRumbleDirector.h"
#include "al/Library/Controller/PadRumbleFunction.h"
#include "al/Library/Framework/GameFrameworkNx.h"
#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorPoseKeeper.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/Memory/HeapUtil.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "al/Library/Player/PlayerUtil.h"
#include "al/Library/Scene/SceneUtil.h"
#include "al/Library/Screen/ScreenFunction.h"
#include "al/Library/System/SystemKit.h"

#include "agl/common/aglDrawContext.h"

#include "game/Item/ShineInfo.h"
#include "game/Player/HackCap.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Player/PlayerAnimator.h"
#include "game/Player/PlayerAnimFrameCtrl.h"
#include "game/Player/PlayerHackKeeper.h"
#include "game/Scene/StageScene.h"
#include "game/Sequence/HakoniwaSequence.h"
#include "game/Sequence/SequenceInitInfo.h"
#include "game/System/Application.h"
#include "game/System/GameDataFile.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolderAccessor.h"

#include <cstring>
#include <math.h>

#include "actors/PuppetActor.h"
#include "gfx/seadColor.h"
#include "heap/seadExpHeap.h"
#include "helpers.hpp"
#include "hooks.hpp"
#include "imgui.h"
#include "Imgui.hpp"
#include "layouts/PlayerEventLog.h"
#include "layouts/SpeedrunIcon.h"
#include "Library/Base/StringUtil.h"
#include "Library/LiveActor/ActorInitInfo.h"
#include "MapObj/CheckpointFlag.h"
#include "prim/seadSafeString.h"
#include "puppetHooks.hpp"
#include "puppets/PuppetInfo.h"
#include "puppets/PuppetMain.hpp"
#include "saveManager.h"
#include "Scene/StageSceneStateModConfig.hpp"
#include "server/Client.hpp"
#include "server/DeltaTime.hpp"
#include "speedboot/BootHooks.hpp"
#include "System/GameDataHolderWriter.h"
#include "System/GameSystem.h"
#include "Util/AchievementUtil.h"

// ===== HOOKS =====

HkTrampoline createHeap = [](TrampolineStatic(), al::SystemKit* systemKit, sead::Heap* rootHeap) -> void {
    orig(systemKit, rootHeap);

    gHeap = sead::ExpHeap::create(2_MB, "SMOOPlusHeap", al::getStationedHeap());
    al::addNamedHeap(gHeap, "SMOOPlusHeap");
};

HkTrampoline gameSystemInit = [](TrampolineStatic(), GameSystem* gameSystem) -> void {
    imgui::setup();

    nn::nifm::Initialize();
    nn::nifm::SubmitNetworkRequest();

    while (nn::nifm::IsNetworkRequestOnHold()) {
    }

    nn::socket::Initialize(socketPool, socketPoolSize, socketAllocPoolSize, 0xE);
    disableSocketInit.installAtSym<"_ZN2nn6socket10InitializeEPvmmi">();

#if DEBUGLOG
    Logger::createInstance();
#endif

    Client::createInstance(gHeap);
    SaveManager::createInstance(gHeap);

    hk::diag::logLine("origing gamesystem init");

    orig(gameSystem);

    hk::diag::logLine("origed successfully yay");
};

HkTrampoline drawMainHookHk = [](TrampolineStatic(), GameSystem* gameSystem) -> void {
    orig(gameSystem);

    auto* drawContext = Application::instance()->mDrawSystemInfo->drawContext;

    /* ImGui */

    ImGui::NewFrame();
    drawMain(gameSystem->mSequence);

    ImGui::Render();

    hk::gfx::ImGuiBackendNvn::instance()->draw(ImGui::GetDrawData(),
                                               drawContext->getCommandBuffer()->ToData()->pNvnCommandBuffer);
};

HkTrampoline initMarioModelActorHook = [](TrampolineStatic(), al::LiveActor* actor,
                                          al::ActorInitInfo& initInfo, char* bodyModel, char* capModel,
                                          al::AudioKeeper* keeper, bool isCloset) -> PlayerCostumeInfo* {
    Client::sendCostumeInfPacket(bodyModel, capModel);
    return orig(actor, initInfo, bodyModel, capModel, keeper, isCloset);
};

HkTrampoline sendShinePacketHook = [](TrampolineStatic(), GameDataHolderWriter writer,
                                      ShineInfo* info) -> void {
    if (!GameDataFunction::isGotShine(writer, info)) {
        for (int x = 0; x < 0x400; x++) {
            GameDataFile::HintInfo* curInfo = &writer->getGameDataFile()->getHintList()[x];
            if (info->mStageName == curInfo->stageName && info->mObjId == curInfo->objId) {
                Client::sendShineCollectPacket(curInfo->uniqueId);

                PlayerEventLog::addSelfEvent(PlayerEventLog::SHINE, PlayerEventLog::getShineMessage(
                                                                        curInfo->stageName, curInfo->objId));
            }
        }
    }
    orig(writer, info);
};

HkTrampoline sendShinePacketHook2 = [](TrampolineStatic(), GameDataFile* file, const char* name) -> void {
    if (!rs::checkGetAchievement(file->getGameDataHolder(), name)) {
        for (int i = 0; i < hk::util::arraySize(toadetteMoons); i++) {
            if (strcmp(toadetteMoons[i], name) == 0) {
                Client::sendShineCollectPacket(2000 + i);

                PlayerEventLog::addSelfEvent(PlayerEventLog::SHINE,
                                             PlayerEventLog::getAchievementMessage(name));
            }
        }
    }

    orig(file, name);
};

HkTrampoline sendCoinCollectCollectPacketHook = [](TrampolineStatic(), GameDataFile* file,
                                                   al::PlacementId* placeID) -> void {
    al::StringTmp<128> placeIDString;
    placeID->makeString(&placeIDString);
    Client::sendCoinCollectCollectPacket(placeIDString.cstr(), file->getCurrentWorldIdNoDevelop(),
                                         file->getStageNameCurrent());

    PlayerEventLog::addSelfEvent(PlayerEventLog::PURPLE, worldNames[file->getCurrentWorldIdNoDevelop()]);

    orig(file, placeID);
};

HkTrampoline sendCheckpointGetPacketHook = [](TrampolineStatic(), CheckpointFlag* checkpoint) -> void {
    if (al::isFirstStep(checkpoint)) {
        al::StringTmp<128> placementId = al::makeStringPlacementId(checkpoint->getPlacementId());
        Client::sendCheckpointGetPacket(placementId.cstr());

        PlayerEventLog::addSelfEvent(PlayerEventLog::CHECKPOINT,
                                     PlayerEventLog::getCheckpointMessage(placementId));
    }
    orig(checkpoint);
};

HkTrampoline hakoniwaSequenceInitHook = [](TrampolineStatic(), HakoniwaSequence* sequence,
                                           al::SequenceInitInfo* initInfo) -> void {
    orig(sequence, initInfo);
    // was threadInit (hook for initializing client class)
    al::LayoutInitInfo lytInfo;

    al::initLayoutInitInfo(&lytInfo, sequence->mLayoutKit, 0, sequence->mAudioDirector,
                           initInfo->mSystemInfo->layoutSystem, initInfo->mSystemInfo->messageSystem,
                           initInfo->mSystemInfo->gamePadSystem);

    Client::instance()->init(lytInfo, sequence->mGameDataHolderAccessor);
};

HkTrampoline initActorInitInfoHook = [](TrampolineStatic(), al::ActorInitInfo* initInfo, al::Scene* scene,
                                        al::PlacementInfo* placementInfo, al::LayoutInitInfo* layoutInfo,
                                        al::ActorFactory* actorFactory, al::SceneMsgCtrl* sceneMsgCtrl,
                                        al::GameDataHolderBase* gameDataHolderBase) -> void {
    orig(initInfo, scene, placementInfo, layoutInfo, actorFactory, sceneMsgCtrl, gameDataHolderBase);

    if (!scene || !al::isEqualString(scene->mName.cstr(), "StageScene"))
        return;

    // was stage init hook
    gIsSceneAlive = true;

    Client::sendGameInfPacket(scene);

    for (s32 i = 0; i < (Client::getMaxPlayerCount() - 1); i++) {
        createPuppetActorFromFactory(*initInfo, false);
    }
};

HkTrampoline sceneKillHook = [](TrampolineStatic(), StageScene* scene) -> void {
    // this hook should prevent crashes on scene transitions
    gIsSceneAlive = false;

    Client::clearArrays();

    orig(scene);
};

HkTrampoline hakoniwaSequenceHook = [](TrampolineStatic(), HakoniwaSequence* sequence) -> void {
    StageScene* stageScene = (StageScene*)sequence->mCurrentScene;

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

    if (gIsSceneAlive)
        updatePlayerInfo(GameDataHolderWriter(stageScene), playerBase, isYukimaru);

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
        if (debugMode && al::isPadTriggerLeft()) {
            GameDataFile::FixedHeapArray<s32, sNumWorlds> scenNumArr =
                Client::sInstance->getHolder()->getGameDataFile()->getScenarioNumArr();

            for (s32 i = 0; i < 17; i++) {
                hk::diag::logLine("%s: Scenario: %d", worldNames[i], scenNumArr[i]);
            }

            hk::diag::logLine("Current Scenario: %d",
                              Client::instance()->getHolder()->getGameDataFile()->getScenarioNo());
        }
    }
    if (Client::isMusicDisabled()) {
        if (al::isPlayingBgm(stageScene)) {
            al::stopAllBgm(stageScene, 0);
        }
    }

    orig(sequence);
};

// ===== PLAYER INFO UPDATE FUNCTION =====

void updatePlayerInfo(GameDataHolderAccessor holder, PlayerActorBase* playerBase, bool isYukimaru) {
    if (pInfSendTimer >= 1) {
        Client::sendPlayerInfPacket(playerBase, isYukimaru);

        if (!isYukimaru) {
            Client::sendHackCapInfPacket(((PlayerActorHakoniwa*)playerBase)->mHackCap);

            Client::sendCaptureInfPacket((PlayerActorHakoniwa*)playerBase);
        }

        pInfSendTimer = 0;
    }

    if (gameInfSendTimer >= 60) {
        if (isYukimaru) {
            Client::sendGameInfPacket(holder);
        } else {
            Client::sendGameInfPacket((PlayerActorHakoniwa*)playerBase, holder);
        }

        /*if (Client::isNeedUpdateHealthCoins()) {
            PlayerHitPointData* data = holder->getGameDataFile()->getPlayerHitPointData();
            data->mIsKidsMode = Client::shouldKids();
            data->mCurrentHealth = Client::getHealth();
            Client::setNeedUpdateHealthCoins(false);
        }*/

        gameInfSendTimer = 0;
    }

    pInfSendTimer++;
    gameInfSendTimer++;
}

// ===== MAIN DRAW HOOK =====

void drawMain(al::Sequence* curSequence) {
    Time::calcTime();

    int dispHeight = al::getLayoutDisplayHeight();

    // Get scene and basic info
    al::Scene* curScene = curSequence->mCurrentScene;
    Client* client = Client::instance();
    SocketClient* socket = client->mSocket;
    bool isConnected = socket->isConnected();

    // ===== NON-DEBUG MODE EXIT =====
    if (!debugMode) {
        if (PlayerEventLog::sInstance)
            PlayerEventLog::sInstance->update();
        return;
    }

    ImGui::Begin("Debug Menu", nullptr,
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar);

    ImGui::SetWindowPos(ImVec2(0, dispHeight / 3.f), ImGuiCond_FirstUseEver);
    ImGui::SetWindowSize(ImVec2(al::getLayoutDisplayWidth() / 3.f, dispHeight - (dispHeight / 4.f)));
    // ===== DEBUG MODE RENDERING =====

    ImGui::Text("FPS: %.1f\n", Application::sInstance->mGameFramework->calcFps());
    // Server info
    if (Client::isServerHidden()) {
        ImGui::Text(isConnected ? "Server: <hidden> | %d/%d Players\n" : "Server: <hidden>\n",
                    isConnected ? Client::getConnectCount() + 1 : 0,
                    isConnected ? Client::getMaxPlayerCount() : 0);
    } else {
        ImGui::Text(isConnected ? "Server: %s:%d | %d/%d Players\n" : "Server: %s:%d\n", socket->getIP(),
                    socket->getPort(), isConnected ? Client::getConnectCount() + 1 : 0,
                    isConnected ? Client::getMaxPlayerCount() : 0);
    }
    ImGui::Text("Your TCP status: %s\n", socket->getStateChar());

    // Queue info
    ImGui::Text("Queue Count: %d/%d (Send) %d/%d (Receive)\n", socket->getSendCount(),
                socket->getSendMaxCount(), socket->getRecvCount(), socket->getRecvMaxCount());

    ImGui::Text("Framework: Hakkun");
    ImGui::Text("Mod version: %s\n", TOSTRING(BUILDVERSTR));
    ImGui::Text("Server is running version: %s\n", Client::getServerVersion());

    // ===== 3D DEBUG RENDERING =====
    if (curScene && gIsSceneAlive) {
        sead::LookAtCamera* cam = &const_cast<sead::LookAtCamera&>(al::getLookAtCamera(curScene, 0));
        sead::Projection* projection =
            cam ? &const_cast<sead::Projection&>(al::getProjectionSead(curScene, 0)) : nullptr;

        if (cam && projection) {
            PlayerActorBase* playerBase = (PlayerActorBase*)rs::getPlayerActor(curScene);
            PuppetActor* curPuppet = Client::getPuppet(debugPuppetIndex - 1);
            PuppetActor* debugPuppet = Client::getDebugPuppet();

            if (debugPuppet) {
                curPuppet = debugPuppet;
            }

            sead::PrimitiveRenderer* renderer = sead::PrimitiveRenderer::instance();
            renderer->mDrawer.setDrawContext(Application::instance()->mDrawSystemInfo->drawContext);
            renderer->setCamera(*cam);
            renderer->setProjection(*projection);

            ImGui::Text("(ZR ←)------------ Page %d/%d -------------(ZR →)\n", pageIndex + 1, maxPages);

            switch (pageIndex) {
            case 0: {
                ImGui::Text("(ZL ←)----------%s Player %d/%d %s-----------(ZL →)\n\n",
                            debugPuppetIndex + 1 < 10 ? "-" : "", debugPuppetIndex + 1,
                            Client::getMaxPlayerCount(), Client::getMaxPlayerCount() < 10 ? "-" : "");

                if (debugPuppetIndex == 0) {
                    ImGui::Text("Player Name: %s\n", Client::getClientName());
                    ImGui::Text("Connection Status: %s\n", isConnected ? "Online" : "Offline");
                    ImGui::Text("Is in same Stage: Yes\n");
                    ImGui::Text("Stage: %s\n", client->getLastGameInfPacket()->stageName);
                    ImGui::Text("Scenario: %u\n", client->getLastGameInfPacket()->scenarioNo);
                    ImGui::Text("Costume: H: %s B: %s\n", client->getLastCostumeInfPacket()->capModel,
                                client->getLastCostumeInfPacket()->bodyModel);
                    ImGui::Text("Capture: %s\n", client->getLastCaptureInfPacket()->hackName);

                    PlayerHackKeeper* hackKeeper = playerBase->getPlayerHackKeeper();
                    if (hackKeeper) {
                        PlayerActorHakoniwa* p1 = (PlayerActorHakoniwa*)playerBase;
                        if (hackKeeper->mHackActor && p1 && isInGame && curScene) {
                            ImGui::Text("Animation: %s\n", al::getActionName(hackKeeper->mHackActor));
                        } else {
                            ImGui::Text("Animation: %s\n", p1->mAnimator->mAnimFrameCtrl->getActionName());
                        }
                    }
                } else if (curPuppet) {
                    al::LiveActor* curModel = curPuppet->getCurrentModel();
                    PuppetInfo* curPupInfo = curPuppet->getInfo();

                    if (curModel && curPupInfo) {
                        ImGui::Text("Player Name: %s\n", curPupInfo->puppetName);
                        ImGui::Text("Connection Status: %s\n",
                                    curPupInfo->isConnected ? "Online" : "Offline");
                        GameMode puppetGameMode = static_cast<GameMode>(curPupInfo->gameMode);
                        ImGui::Text("Is in same Stage: %s\n", curPupInfo->isInSameStage ? "Yes" : "No");
                        ImGui::Text("Stage: %s\n", curPupInfo->stageName);
                        ImGui::Text("Scenario: %u\n", curPupInfo->scenarioNo);
                        ImGui::Text("Costume: H: %s B: %s\n", curPupInfo->costumeHead,
                                    curPupInfo->costumeBody);
                        ImGui::Text("Capture: %s\n", curPupInfo->isCaptured ? curPupInfo->curHack : "");
                        ImGui::Text("Animation: %d %s\n", curPupInfo->curAnim, curPupInfo->curAnimStr);

                        const char* modelAnim = al::getActionName(curModel);
                        ImGui::Text("Model Animation: %s\n", modelAnim ? modelAnim : "none");
                        ImGui::Text("Is Capture Model: %s\n", curPupInfo->isCaptured ? "Yes" : "No");
                    }
                }
                break;
            }
            case 1: {
                ImGui::Text("--------------- Animation & Cappy ---------------\n\n");
                PlayerHackKeeper* hackKeeper = playerBase->getPlayerHackKeeper();

                if (hackKeeper) {
                    PlayerActorHakoniwa* p1 = (PlayerActorHakoniwa*)playerBase;

                    if (hackKeeper->mHackActor) {
                        al::LiveActor* curHack = hackKeeper->mHackActor;
                        ImGui::Text("Current Hack Animation: %s\n", al::getActionName(curHack));
                        ImGui::Text("Current Hack Name: %s\n", hackKeeper->getCurrentHackName());

                        sead::Quatf captureRot = curHack->mPoseKeeper->getQuat();
                        ImGui::Text("Current Hack Rot: %.3f %.3f %.3f %f\n", captureRot.x, captureRot.y,
                                    captureRot.z, captureRot.w);

                        sead::Quatf calcRot;
                        al::calcQuat(&calcRot, curHack);
                        ImGui::Text("Calc Hack Rot: %.3f %.3f %.3f %.3f\n", calcRot.x, calcRot.y, calcRot.z,
                                    calcRot.w);
                    } else {
                        ImGui::Text("Cur Action: %s\n", p1->mAnimator->mAnimFrameCtrl->getActionName());
                        ImGui::Text("Cur Anim: %s\n", p1->mAnimator->mCurAnim.cstr());
                        ImGui::Text("Cur Sub Anim: %s\n", p1->mAnimator->mCurSubAnim.cstr());
                        ImGui::Text("Is Cappy Flying? %s\n", BTOC(p1->mHackCap->isFlying()));

                        if (p1->mHackCap->isFlying()) {
                            ImGui::Text("Cappy Action: %s\n", al::getActionName(p1->mHackCap));
                            sead::Vector3f* capTrans = al::getTransPtr(p1->mHackCap);
                            sead::Vector3f* capRot = &p1->mHackCap->mJointKeeper->mJointRot;
                            ImGui::Text("Cappy: Position   Rotation\nX:   % 10.3f % 10.3f\nY:   % 10.3f % "
                                        "10.3f\nZ:   % 10.3f % 10.3f\n",
                                        capTrans->x, capRot->x, capTrans->y, capRot->y, capTrans->z,
                                        capRot->z);
                            ImGui::Text("Cappy Skew: %.3f\n", p1->mHackCap->mJointKeeper->mSkew);
                        }
                    }
                }
                break;
            }
            case 2: {
                ImGui::Text("------------------- Heaps --------------------\n\n");

                auto displayHeapInfo = [](sead::Heap* heap, const char* heapName, bool isKB = false) {
                    if (!heap) {
                        return;
                    }

                    ImGui::Text("%s   ", heapName);
                    ImGui::SameLine();

                    float used = isKB ? (heap->getSize() - heap->getFreeSize()) / float(1_KB) :
                                        (heap->getSize() - heap->getFreeSize()) / float(1_MB);
                    float max = isKB ? heap->getSize() / float(1_KB) : heap->getSize() / float(1_MB);
                    float percentUsed =
                        (heap->getSize() - heap->getFreeSize()) / (float(heap->getSize()) / 100);
                    char buf[0x20];
                    snprintf(buf, sizeof(buf), "%.3f/%.3f %s", used, max, isKB ? "KB" : "MB");

                    ImGui::ProgressBar(percentUsed / 100, ImVec2(-1, 0), buf);
                };

                displayHeapInfo(gHeap, "SMOOPlus");
                displayHeapInfo(al::getStationedHeap(), "Stationed");
                displayHeapInfo(al::getSequenceHeap(), "Sequence");
                displayHeapInfo(al::getSceneHeap(), "Scene");
                displayHeapInfo(al::getSceneResourceHeap(), "SceneResource", true);
                displayHeapInfo(al::getWorldResourceHeap(), "WorldResource");

                break;
            }
            case 3: {
                ImGui::Text("------------------- Controls --------------------\n\n");

                ImGui::Text("- ZR + ↑ | Open/close this debug menu\n");
                ImGui::Text("- ZL + ↑ | Open/close the player event log\n");
                break;
            }
            default:
                break;
            }

            // Render 3D debug spheres
            renderer->begin();
            renderer->setModelMatrix(sead::Matrix34f::ident);

            if (curPuppet) {
                renderer->drawSphere4x8(curPuppet->getInfo()->playerPos, 20,
                                        sead::Color4f(1.f, 0.f, 0.f, 0.25f));
                renderer->drawSphere4x8(al::getTrans(curPuppet), 20, sead::Color4f(0.f, 0.f, 1.f, 0.25f));
            } else if (debugPuppetIndex == 0) {
                renderer->drawSphere4x8(client->getLastPlayerInfPacket()->playerPos, 20,
                                        sead::Color4f(1.f, 0.f, 0.f, 0.25f));
            }

            renderer->end();
        }
        isInGame = false;
        ImGui::End();
        return;
    }

    ImGui::End();
}

// ===== LOGGING HOOK =====

void seadPrintHook(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Logger::disableName();
    hk::diag::log(fmt, args);
    Logger::enableName();
    va_end(args);
}

HkReplaceVarArgs replaceSeadPrintHook = seadPrintHook;

extern "C" void hkMain() {
    // Init Stuff
    createHeap.installAtSym<"_ZN2al9SystemKit18createMemorySystemEPN4sead4HeapE">();
    gameSystemInit.installAtSym<"_ZN10GameSystem4initEv">();
    hakoniwaSequenceInitHook.installAtSym<"_ZN16HakoniwaSequence4initERKN2al16SequenceInitInfoE">();
    initActorInitInfoHook.installAtSym<"R_ZN2al17initActorInitInfo">();

    // Debug Stuff
    drawMainHookHk.installAtSym<"_ZN10GameSystem8drawMainEv">();
    replaceSeadPrintHook.installAtSym<"_ZN4sead6system5PrintEPKcz">();

    // Main Stuff
    hakoniwaSequenceHook.installAtSym<"_ZN16HakoniwaSequence12exePlayStageEv">();
    initMarioModelActorHook.installAtSym<"R_ZN14PlayerFunction19initMarioModelActor">();

    // Shine Syncing
    sendShinePacketHook
        .installAtSym<"_ZN16GameDataFunction11setGotShineE20GameDataHolderWriterPK9ShineInfo">();
    sendShinePacketHook2.installAtSym<"_ZN12GameDataFile14getAchievementEPKc">();
    registerShineToListHook.installAtSym<"_ZN5Shine18initAfterPlacementEv">();

    // CoinCollect Syncing
    sendCoinCollectCollectPacketHook.installAtSym<"_ZN12GameDataFile14addCoinCollectEPKN2al11PlacementIdE">();
    registerCoinCollectToListHook
        .installAtSym<"_ZN17CoinCollectHolder19registerCoinCollectEP11CoinCollect">();
    registerCoinCollect2DToListHook
        .installAtSym<"_ZN17CoinCollectHolder21registerCoinCollect2DEP13CoinCollect2D">();

    // CheckpointFlag Syncing
    sendCheckpointGetPacketHook.installAtSym<"_ZN14CheckpointFlag6exeGetEv">();

    // Amiibo Button Disabling
    hk::hook::replace([]() -> void {
        return;
    }).installAtSym<"_ZN2rs16isHoldAmiiboModeEPKN2al18IUseSceneObjHolderE">();
    hk::hook::replace([]() -> void {
        return;
    }).installAtSym<"_ZN2rs19isTriggerAmiiboModeEPKN2al18IUseSceneObjHolderE">();

    // Capture Syncing
    initObjHook
        .installAtSym<"_ZN2al31createPlacementActorFromFactoryERKNS_13ActorInitInfoEPKNS_13PlacementInfoE">();

    // Save Data Edits
    saveWriteHook.installAtSym<"_ZN14GameConfigData5writeEPN2al11ByamlWriterE">();
    saveReadHook.installAtSym<"_ZN14GameConfigData4readERKN2al9ByamlIterE">();

    // WindowConfirm Edits (Forces logic to ignore current nerve)
    windowConfirmWaitHook.installAtSym<"_ZN2al17WindowConfirmWait6tryEndEv">();

    // Pause Menu Changes

    hk::hook::a64::assemble<"mov w2, #5">()
        .installAtSym<"R_ZN24StageSceneStatePauseMenuNrvStateCount">();     // increase nerve state count to 5
    initNerveStateHook.installAtSym<"R_ZN24StageSceneStatePauseMenuC1">();  // inits options nerve state and
                                                                            // server config state
    pauseMenuAppearHook
        .installAtSym<"_ZN24StageSceneStatePauseMenu9exeAppearEv">();  // Change Action Guide Text

    pauseMenuWaitHook.installAtSym<"_ZN24StageSceneStatePauseMenu7exeWaitEv">();  // Onine Indicator

    // inits StageSceneStateOption and StageSceneStateModConfig
    initStateHook.installAtSym<"_ZN21StageSceneStateOptionC1EPKcPN2al5SceneERKNS2_"
                               "14LayoutInitInfoEP11FooterPartsP14GameDataHolderb">();
    overrideHelpFadeNerve.installAtSym<"_ZN24StageSceneStatePauseMenu17exeFadeBeforeHelpEv">();

    // custom bootscreen hooks
    hk::hook::writeBranchLinkAtSym<"R_hakoniwaSetNerveSetup">(speedboot::hakoniwaSetNerveSetup);
    hk::hook::a64::assemble<"mov w2, #0x1f">()
        .installAtSym<"R_hakoniwaSetNerveCount">();  // nerve state count
    speedboot::prepareSpeedBootHook.installAtSym<"_ZN10BootLayoutC1ERKN2al14LayoutInitInfoE">();

    // unlock costume doors
    unlockCostumeDoorsHook.installAtSym<
        "_ZN2al19listenStageSwitchOnEPNS_15IUseStageSwitchEPKcRKNS_11FunctorBaseE">();  // all except metro
    hk::hook::writeBranchLinkAtSym<"R_metroCostumeDoor">(unlockCostumeDoorMetroHook);   // metro

    // QOL Patches (disabled because these cs skips arent in freeze tag)
    // hk::hook::a64::assemble<"nop">().installAtMainOffset(0x4DB934);  // LifeUpMaxItem demo skip
    // hk::hook::a64::assemble<"nop">().installAtMainOffset(0x2D250C);  // Notes Demo Skip

    hk::hook::trampoline([]() -> void {
    }).installAtSym<"_ZN2rs21requestShowHtmlViewerEPKN2al18IUseSceneObjHolderE">();  // Disable Action Guide /
                                                                                     // HtmlViewer
    disableAppearSwitchCameraHook
        .installAtSym<"R_ZN17AppearSwitchTimer4init">();  // disables AppearSwitchTimer's camera switch
    hk::hook::a64::assemble<"nop">().installAtMainOffset(0x45c69c);  // Removes Assist Mode Ledge Grabs

    hk::hook::trampoline([]() -> bool {
        return true;
    }).installAtSym<"_ZNK9MapLayout22isEnableCheckpointWarpEv">();

    // World Resource Heap stuff
    // hk::ro::getMainModule()->writeRo(0x5145c8, 0x7107D29F);  // cmp w20, #500
    // hk::hook::a64::assemble<"ret">().installAtMainOffset(0x514710);

    startNewGameHook.installAtSym<"_ZN24HakoniwaStateDemoOpening7exeLoadEv">();

    moonRockHook.installAtSym<"_ZN8MoonRock11exeReactionEv">();

    mountSdCardHook.installAtSym<"_ZN4sead13FileDeviceMgrC1Ev">();

    sceneKillHook.installAtSym<"_ZN10StageScene4killEv">();

    hk::gfx::ImGuiBackendNvn::instance()->installHooks(false);
}

namespace nn::init {

extern "C" void _init_libc0();
extern "C" void nnosInitialize(hk::Handle threadHandle, ptr argumentAddr);
extern "C" void _init_libc1();
extern "C" void _init_libc2();
extern "C" void nnMain();
extern "C" void nnosQuickExit();

extern "C" __attribute__((weak)) void nninitInitializeSdkModule(void);
extern "C" __attribute__((weak)) void nninitInitializeAbortObserver(void);
extern "C" __attribute__((weak)) void nninitFinalizeSdkModule(void);

using FuncPtr = void (*)();

void nninitStartup() {  // Copied straight from OdysseyDecomp with some slight adjustments
    uintptr_t allocatorHeap;
    uintptr_t recordingHeap;

    nn::os::SetMemoryHeapSize(3200_MB + extraRAMAmount);
    nn::os::AllocateMemoryBlock(&allocatorHeap, 36_MB);
    nn::init::InitializeAllocator(reinterpret_cast<void*>(allocatorHeap), 36_MB);
    nn::os::AllocateMemoryBlock(&recordingHeap, 96_MB);
    nn::oe::EnableGamePlayRecording(reinterpret_cast<void*>(recordingHeap), 96_MB);
}

void Start(size threadHandle, size argumentAddr, FuncPtr notifyExceptionHandlerReady,
           FuncPtr callInitializers) {
    _init_libc0();
    nnosInitialize(threadHandle, argumentAddr);

    // (*notifyExceptionHandlerReady)();

    _init_libc1();
    nninitInitializeSdkModule();

    nninitStartup();
    _init_libc2();
    (*callInitializers)();

    uint s2 = 3068_MB + extraRAMAmount;
    hk::hook::a64::assemble<"mov w8,{}">().arg(s2).installAtMainOffset(0x005157b8);

    nnMain();

    nninitFinalizeSdkModule();
    nnosQuickExit();
    return;
}

}  // namespace nn::init