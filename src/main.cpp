/**
 * @file main.cpp
 * @brief Core game hooks and main functionality for the multiplayer client
 */

#include "main.hpp"

#include "hk/gfx/ImGuiBackendNvn.h"

#include "sead/gfx/seadColor.h"
#include "sead/prim/seadSafeString.h"
#include <sead/gfx/seadCamera.h>
#include <sead/gfx/seadPrimitiveRenderer.h>
#include <sead/gfx/seadProjection.h>
#include <sead/heap/seadHeap.h>
#include <sead/prim/seadStringUtil.h>

#include "al/Library/Bgm/BgmLineFunction.h"
#include "al/Library/Camera/CameraUtil.h"
#include "al/Library/Controller/PadRumbleDirector.h"
#include "al/Library/Controller/PadRumbleFunction.h"
#include "al/Library/Framework/GameFrameworkNx.h"
#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorInitInfo.h"
#include "al/Library/LiveActor/ActorPoseKeeper.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/Memory/HeapUtil.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "al/Library/Player/PlayerUtil.h"
#include "al/Library/Scene/SceneUtil.h"
#include "al/Library/Screen/ScreenFunction.h"
#include "al/Library/System/GameSystemInfo.h"

#include "agl/common/aglDrawContext.h"  // IWYU pragma: keep

#include "game/Item/ShineInfo.h"
#include "game/Player/HackCap.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Player/PlayerAnimator.h"
#include "game/Player/PlayerAnimFrameCtrl.h"
#include "game/Player/PlayerHackKeeper.h"
#include "game/Scene/StageScene.h"
#include "game/Sequence/HakoniwaSequence.h"
#include "game/System/Application.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolderAccessor.h"
#include "game/System/GameSystem.h"
#include "game/Util/AchievementUtil.h"

#include "actors/PuppetActor.h"
#include "imgui.h"
#include "layouts/PlayerEventLog.h"
#include "puppets/PuppetInfo.h"
#include "server/Client.hpp"
#include "server/DeltaTime.hpp"
#include "speedboot/BootHooks.hpp"
#include "types.h"

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
    ImGui::Text("Mod version: %s: %s\n", TOSTRING(BUILDVERSTR), __DATE__);

    // ===== 3D DEBUG RENDERING =====
    if (curScene && gIsSceneAlive) {
        sead::LookAtCamera* cam = &const_cast<sead::LookAtCamera&>(al::getLookAtCamera(curScene, 0));
        sead::Projection* projection =
            cam ? &const_cast<sead::Projection&>(al::getProjectionSead(curScene, 0)) : nullptr;

        if (cam && projection) {
            PlayerActorBase* playerBase = (PlayerActorBase*)rs::getPlayerActor(curScene);
            PuppetActor* curPuppet = Client::getPuppet(debugPuppetIndex - 1);

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

                auto displayHeapInfo = [](sead::Heap* heap, bool isKB = false) {
                    if (!heap) {
                        return;
                    }

                    ImGui::Text("%s   ", heap->getName().cstr());
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

                displayHeapInfo(gHeap);
                displayHeapInfo(al::getStationedHeap());
                displayHeapInfo(al::getSequenceHeap());
                displayHeapInfo(al::getSceneHeap());
                displayHeapInfo(al::getSceneResourceHeap(), true);
                displayHeapInfo(al::getWorldResourceHeap());

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

extern "C" void hkMain() {
    installSyncHooks();
    installInitHooks();
    installModMenuHooks();
    installQolHooks();
    installOtherHooks();
    speedboot::installSpeedbootHooks();

    hk::gfx::ImGuiBackendNvn::instance()->installHooks(false);
}
