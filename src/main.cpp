/**
 * @file main.cpp
 * @brief Core game hooks and main functionality for the multiplayer client
 */

#include "main.hpp"

#include "hk/gfx/DebugRenderer.h"
#include "hk/gfx/Util.h"
#include "hk/gfx/Vertex.h"
#include "hk/hook/a64/Assembler.h"
#include "hk/hook/InstrUtil.h"
#include "hk/hook/Trampoline.h"
#include "hk/util/Math.h"

#include "al/Library/Bgm/BgmLineFunction.h"
#include "al/Library/Camera/CameraUtil.h"
#include "al/Library/Controller/InputFunction.h"
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
#include "al/Project/Memory/Util.h"

#include "agl/common/aglDrawContext.h"

#include "sead/gfx/seadCamera.h"
#include "sead/gfx/seadPrimitiveRenderer.h"
#include "sead/gfx/seadProjection.h"
#include "sead/heap/seadHeap.h"

#include "game/Info/ShineInfo.h"
#include "game/Player/HackCap.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Player/PlayerAnimator.h"
#include "game/Player/PlayerHackKeeper.h"
#include "game/Scene/StageScene.h"
#include "game/Sequence/HakoniwaSequence.h"
#include "game/Sequence/SequenceInitInfo.h"
#include "game/System/Application.h"
#include "game/System/GameDataFile.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameDataHolderAccessor.h"
#include "game/System/PlayerHitPointData.h"

#include <cmath>
#include <cstring>
#include <math.h>

#include "actors/PuppetActor.h"
#include "factoryPatches.h"
#include "gfx/seadColor.h"
#include "helpers.hpp"
#include "hooks.hpp"
#include "hooksFreezeTag.hpp"
#include "imgui.h"
#include "Imgui.hpp"
#include "layouts/ConnectionStatus.h"
#include "layouts/SpeedrunIcon.h"
#include "logger.hpp"
#include "nn/hid.h"
#include "nn/socket.h"
#include "puppetHooks.hpp"
#include "puppets/PuppetInfo.h"
#include "server/Client.hpp"
#include "server/DeltaTime.hpp"
#include "server/freeze/FreezeTagMode.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "Settings/SmooSettings.hpp"
#include "Settings/StageWarper.hpp"
#include "speedboot/BootHooks.hpp"
#include "System/GameSystem.h"
#include "TwistsConfig.hpp"
#include "Util/AchievementUtil.h"

// ===== GLOBAL VARIABLES =====
static int pInfSendTimer = 0;
static int gameInfSendTimer = 0;
static int debugPuppetIndex = 0;
static int debugCaptureIndex = 0;
static int pageIndex = 0;
static const int maxPages = 4;

al::SequenceInitInfo* initInfo;

static constexpr int socketPoolSize = 0x600000;
static constexpr int socketAllocPoolSize = 0x20000;
char socketPool[socketPoolSize + socketAllocPoolSize] __attribute__((aligned(0x1000)));
HkTrampoline<void> disableSocketInit = hk::hook::trampoline([]() -> void {});

// ===== HOOKS =====

HkTrampoline<void, GameSystem*> gameSystemInit = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
    imgui::setup();

    nn::socket::Initialize(socketPool, socketPoolSize, socketAllocPoolSize, 0xE);
    disableSocketInit.installAtSym<"_ZN2nn6socket10InitializeEPvmmi">();
#if DEBUGLOG
    Logger::createInstance();
#endif

    Client::createInstance(al::getCurrentHeap());
    GameModeManager::createInstance(al::getCurrentHeap());

    gameSystemInit.orig(gameSystem);

    // nn::hid::InitializeMouse();
    // nn::hid::InitializeKeyboard();
});

HkTrampoline<void, GameSystem*> drawMainHookHk = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
    drawMainHookHk.orig(gameSystem);

    auto* drawContext = Application::instance()->mDrawSystemInfo->drawContext;

    /* ImGui */

    // imgui::updateImGuiInput();

    ImGui::NewFrame();
    drawMain(gameSystem->mSequence);
    StageWarper::ShowSearchWindow();
    ImGui::Render();

    hk::gfx::ImGuiBackendNvn::instance()->draw(ImGui::GetDrawData(), drawContext->getCommandBuffer()->ToData()->pNvnCommandBuffer);
});

HkTrampoline<PlayerCostumeInfo*, al::LiveActor*, al::ActorInitInfo&, char*, char*, al::AudioKeeper*, bool> initMarioModelActorHook = hk::hook::trampoline(
    [](al::LiveActor* actor, al::ActorInitInfo& initInfo, char* bodyModel, char* capModel, al::AudioKeeper* keeper, bool isCloset) -> PlayerCostumeInfo* {
        Client::sendCostumeInfPacket(bodyModel, capModel);
        return initMarioModelActorHook.orig(actor, initInfo, bodyModel, capModel, keeper, isCloset);
    });

HkTrampoline<void, GameDataHolderWriter, ShineInfo*> sendShinePacketHook = hk::hook::trampoline([](GameDataHolderWriter writer, ShineInfo* info) -> void {
    if (!GameDataFunction::isGotShine(writer, info)) {
        for (int x = 0; x < 0x400; x++) {
            GameDataFile::HintInfo* curInfo = &writer->getGameDataFile()->mShineHintList[x];
            if (info->stageName == curInfo->mStageName && info->objectId == curInfo->mObjId) {
                Client::sendShineCollectPacket(curInfo->mUniqueID);
            }
        }
    }
    sendShinePacketHook.orig(writer, info);
});

HkTrampoline<void, GameDataFile*, const char*> sendShinePacketHook2 = hk::hook::trampoline([](GameDataFile* file, const char* name) -> void {
    if (!rs::checkGetAchievement(file->mGameDataHolder, name)) {
        for (int i = 0; i < hk::util::arraySize(toadetteMoons); i++) {
            if (strcmp(toadetteMoons[i], name) == 0) {
                Client::sendShineCollectPacket(2000 + i);
            }
        }
    }

    sendShinePacketHook2.orig(file, name);
});

HkTrampoline<void, GameDataFile*, al::PlacementId*> sendCoinCollectCollectPacketHook = hk::hook::trampoline([](GameDataFile* file, al::PlacementId* placeID) -> void {
    al::StringTmp<128> placeIDString;
    placeID->makeString(&placeIDString);
    Client::sendCoinCollectCollectPacket(placeIDString.cstr(), file->getCurrentWorldIdNoDevelop(), file->mCurrentStageName.cstr());
    sendCoinCollectCollectPacketHook.orig(file, placeID);
});

HkTrampoline<void, HakoniwaSequence*, al::SequenceInitInfo*> hakoniwaSequenceInitHook =
    hk::hook::trampoline([](HakoniwaSequence* sequence, al::SequenceInitInfo* initInfo) -> void {
        hakoniwaSequenceInitHook.orig(sequence, initInfo);
        // was threadInit ( hook for initializing client class)
        al::LayoutInitInfo lytInfo;

        al::initLayoutInitInfo(&lytInfo, sequence->mLayoutKit, 0, sequence->mAudioDirector, initInfo->mSystemInfo->layoutSystem,
                               initInfo->mSystemInfo->messageSystem, initInfo->mSystemInfo->gamePadSystem);

        Client::instance()->init(lytInfo, sequence->mGameDataHolderAccessor);

        speedrun::createHooks();

        ConnectionStatus::sInstance = new ConnectionStatus("Status", lytInfo);
        SpeedrunIcon::sInstance = new SpeedrunIcon("SpeedrunIcon", lytInfo);
    });

HkTrampoline<void, al::ActorInitInfo*, al::Scene*, al::PlacementInfo*, al::LayoutInitInfo*, al::ActorFactory*, al::SceneMsgCtrl*, al::GameDataHolderBase*>
    initActorInitInfoHook =
        hk::hook::trampoline([](al::ActorInitInfo* initInfo, al::Scene* scene, al::PlacementInfo* placementInfo, al::LayoutInitInfo* layoutInfp,
                                al::ActorFactory* actorFactory, al::SceneMsgCtrl* sceneMsgCtrl, al::GameDataHolderBase* gameDataHolderBase) -> void {
            initActorInitInfoHook.orig(initInfo, scene, placementInfo, layoutInfp, actorFactory, sceneMsgCtrl, gameDataHolderBase);

            if (!scene || !al::isEqualString(scene->mName.cstr(), "StageScene"))
                return;

            // was stage init hook
            Client::clearArrays();

            Client::setSceneInfo(*initInfo, (StageScene*)scene);

            if (GameModeManager::instance()->getGameMode() != NONE) {
                GameModeInitInfo initModeInfo(initInfo, scene);
                initModeInfo.initServerInfo(GameModeManager::instance()->getGameMode(), Client::getPuppetHolder());

                GameModeManager::instance()->initScene(initModeInfo);
            }

            Client::sendGameInfPacket(initInfo->actorSceneInfo.sceneObjHolder);
            TwistsConfig::handleStageInit();
        });

HkTrampoline<void, HakoniwaSequence*> hakoniwaSequenceHook = hk::hook::trampoline([](HakoniwaSequence* sequence) -> void {
    StageScene* stageScene = (StageScene*)sequence->mCurrentScene;

    static bool isCameraActive = false;

    bool isFirstStep = al::isFirstStep(sequence);

    al::PlayerHolder* pHolder = al::getScenePlayerHolder(stageScene);
    PlayerActorBase* playerBase = (PlayerActorBase*)al::tryGetPlayerActor(pHolder, 0);
    auto* player = (PlayerActorHakoniwa*)al::tryGetPlayerActor(pHolder, 0);

    bool isYukimaru = !playerBase->getPlayerInfo();

    isInGame = !stageScene->isPause();

    GameModeManager::instance()->setPaused(stageScene->isPause());
    Client::setStageInfo(GameDataHolderWriter(stageScene));

    Client::update();

    updatePlayerInfo(GameDataHolderWriter(stageScene), playerBase, isYukimaru);

    TwistsConfig::updateCappyProximity(player, stageScene);

    if (SpeedrunIcon::sInstance) {
        if (StageSceneStateServerConfig::isSpeedrunModeEnabled()) {
            SpeedrunIcon::sInstance->tryStart();
            speedrun::uninstallHooks();
        } else {
            SpeedrunIcon::sInstance->tryEnd();
            speedrun::installHooks();
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
    } else if (al::isPadHoldL(-1)) {
        if (al::isPadTriggerLeft(-1) && !StageSceneStateServerConfig::isSpeedrunModeEnabled()) {
            GameModeManager::instance()->toggleActive();
        }
    }

    if (Client::isMusicDisabled()) {
        if (al::isPlayingBgm(stageScene)) {
            al::stopAllBgm(stageScene, 0);
        }
    }

    if (isFirstStep && GameModeManager::instance()->isMode(GameMode::FREEZETAG))
        GameModeManager::instance()->getMode<FreezeTagMode>()->setWipeHolder(sequence->mWipeHolder);

    hakoniwaSequenceHook.orig(sequence);
});

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

        if (Client::isNeedUpdateHealthCoins()) {
            PlayerHitPointData* data = holder.mData->getGameDataFile()->getPlayerHitPointData();
            data->mIsKidsMode = Client::shouldKids();
            data->mCurrentHealth = Client::getHealth();
            Client::setNeedUpdateHealthCoins(false);
        }

        gameInfSendTimer = 0;
    }

    pInfSendTimer++;
    gameInfSendTimer++;
}

// ===== MAIN DRAW HOOK =====
static int msgCount = 0;

struct DisplayMessage {
    sead::FixedSafeString<MESSAGESIZE> text;
    float displayTimer;
    bool active;
};

static const int maxDisplayMsgCount = 5;
static const float messageDisplayDuration = 10.0f;
static DisplayMessage displayMessages[maxDisplayMsgCount] = {};

void drawMain(al::Sequence* curSequence) {
    GameModeManager* gmm = GameModeManager::instance();
    GameModeBase* mode = gmm->getMode<GameModeBase>();

    // Freeze tag needs the delta time to not update while the game is paused
    if (gmm->isMode(GameMode::FREEZETAG)) {
        if (!gmm->isPaused())
            Time::calcTime();
    } else {
        Time::calcTime();
    }

    int dispHeight = al::getLayoutDisplayHeight();

    // Get scene and basic info
    al::Scene* curScene = curSequence->mCurrentScene;
    Client* client = Client::instance();
    SocketClient* socket = client->mSocket;
    bool isConnected = socket->isConnected();
    bool isPaused = gmm->isPaused();

    // Check authorization
    const char* currentUser = Client::getClientName();
    bool isAuthorizedUser = (strcmp(currentUser, "SrDev") == 0) || (strcmp(currentUser, "Crafty") == 0) || (strcmp(currentUser, "KleinTimmi") == 0) ||
                            (strcmp(currentUser, "Katzen") == 0);

    // ===== CHAT RENDERING (Non-debug mode, in-game only) =====
    if (!debugMode && curScene && isInGame) {
        auto* renderer = hk::gfx::DebugRenderer::instance();
        auto* drawContext = Application::instance()->mDrawSystemInfo->drawContext;

        float deltaTime = Time::deltaTime;
        float baseY = (dispHeight * 7 / 10) + 95.f - 5.f;
        float lineHeight = 30.f;

        // Update display timer and active state
        for (int i = 0; i < maxDisplayMsgCount; i++) {
            if (displayMessages[i].active) {
                displayMessages[i].displayTimer -= deltaTime;
                if (displayMessages[i].displayTimer <= 0.0f) {
                    displayMessages[i].active = false;
                    msgCount--;
                }
            }
        }

        // Add new messages from the queue if there's space
        while (msgCount < maxDisplayMsgCount && Client::get()->getMsgCount() > 0) {
            sead::FixedSafeString<MESSAGESIZE>* msg = Client::get()->tryGetMessage();
            if (msg) {
                // Find an empty slot
                for (int i = 0; i < maxDisplayMsgCount; i++) {
                    if (!displayMessages[i].active) {
                        displayMessages[i].text = *msg;
                        displayMessages[i].displayTimer = messageDisplayDuration;
                        displayMessages[i].active = true;
                        msgCount++;
                        break;
                    }
                }
                Client::getClientHeap()->free(msg);
            }
        }

        renderer->begin(drawContext->getCommandBuffer()->ToData()->pNvnCommandBuffer);
        renderer->clear();

        if (msgCount > 0) {
            float charWidth = 15.f;
            int maxCharCount = 0;
            for (int i = 0; i < maxDisplayMsgCount; i++) {
                if (displayMessages[i].active) {
                    int len = displayMessages[i].text.calcLength();
                    if (len > maxCharCount) {
                        maxCharCount = len;
                    }
                }
            }
            float lineWidth = 10.f + charWidth * maxCharCount + 3.f;

            hk::gfx::Vertex bl = {{0, baseY + lineHeight + 10}, {0, 1.0}, 0xef000000};
            hk::gfx::Vertex tl = {{0, baseY - (msgCount - 1) * lineHeight - 10}, {0, 0}, 0xef000000};
            hk::gfx::Vertex tr = {{lineWidth, baseY - (msgCount - 1) * lineHeight - 10}, {1.0, 0}, 0xef000000};
            hk::gfx::Vertex br = {{lineWidth, baseY + lineHeight + 10}, {1.0, 1.0}, 0xef000000};

            renderer->drawQuad(tl, tr, br, bl);
        }

        // Draw all active messages

        // TODO: Fix Outline
        int displayIndex = 0;
        ImVec2 winPos = ImGui::GetWindowPos();
        float winX = winPos.x;
        float winY = winPos.y;
        for (int i = 0; i < maxDisplayMsgCount; i++) {
            if (displayMessages[i].active) {
                // Position relativ zum Fenster
                float yPos = 20.f + lineHeight * displayIndex;  // 20px Padding vom Fenster-Top
                float xPos = 10.f;                              // Padding von links

                hk::util::Vector2f pos(winX + xPos, winY + yPos);
                hk::util::Vector2f shadowPos = pos + hk::util::Vector2f(2.f, 2.f);

                sead::Color4f color(255, 255, 255, 255);
                u32 coloru32 = hk::gfx::rgba(color.a, color.g, color.b, color.a);
                u8 shadowAlpha = fmax(0.0f, color.a - 25);
                u32 shadowColor = hk::gfx::rgba(0, 0, 0, shadowAlpha);
                renderer->setGlyphHeight(30.f);

                renderer->drawString(shadowPos, displayMessages[i].text.cstr(), shadowColor);

                // then draw text
                renderer->drawString(pos, displayMessages[i].text.cstr(), coloru32);
                displayIndex++;
            }
        }

        renderer->end();

        isInGame = false;

        return;
    }

    // ===== NON-DEBUG MODE EXIT =====
    if (!debugMode)
        return;

    ImGui::Begin("Debug Menu", nullptr,
                 ImGuiWindowFlags_NoSavedSettings /*| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse*/ | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar);

    ImGui::SetWindowPos(ImVec2(0, dispHeight / 3.f), ImGuiCond_FirstUseEver);
    ImGui::SetWindowSize(ImVec2(al::getLayoutDisplayWidth() / 3.f, dispHeight - (dispHeight / 4.f)));
    // ===== DEBUG MODE RENDERING =====

    ImGui::Text("FPS: %d\n", static_cast<int>(round(Application::sInstance->mGameFramework->calcFps())));
    // Server info
    if (Client::isServerHidden()) {
        ImGui::Text(isConnected ? "Server: <hidden> | %d/%d Players\n" : "Server: <hidden>\n", isConnected ? Client::getConnectCount() + 1 : 0,
                    isConnected ? Client::getMaxPlayerCount() : 0);
    } else {
        ImGui::Text(isConnected ? "Server: %s:%d | %d/%d Players\n" : "Server: %s:%d\n", socket->getIP(), socket->getPort(),
                    isConnected ? Client::getConnectCount() + 1 : 0, isConnected ? Client::getMaxPlayerCount() : 0);
    }
    ImGui::Text("Your TCP status: %s\n", socket->getStateChar());

    // Heap info
    sead::Heap* clientHeap = Client::getClientHeap();
    if (clientHeap) {
        sead::Heap* gmHeap = gmm->getHeap();
        if (gmHeap && clientHeap->getSize() > 0 && gmHeap->getSize() > 0) {
            float clientUsed = clientHeap->getSize() - clientHeap->getFreeSize();
            float clientTotal = clientHeap->getSize();
            float gmUsed = gmHeap->getSize() - gmHeap->getFreeSize();
            float gmTotal = gmHeap->getSize();
            float ImguiUsed = imgui::sImGuiHeap->getSize() - imgui::sImGuiHeap->getFreeSize();
            float ImguiTotal = imgui::sImGuiHeap->getSize();

            ImGui::Text("Heap Use: %.1f/%.0f (Client) %.1f/%.0f (Gmode)\n", clientUsed / 1_KB, clientTotal / 1_KB, gmUsed / 1_KB, gmTotal / 1_KB);
        } else {
            ImGui::Text("Heap Use: Invalid heap sizes\n");
        }
    } else {
        ImGui::Text("Heap Use: Client heap unavailable\n");
    }

    // Queue info
    ImGui::Text("Queue Count: %d/%d (Send) %d/%d (Receive) %d/%d (Msg)\n", socket->getSendCount(), socket->getSendMaxCount(), socket->getRecvCount(),
                socket->getRecvMaxCount(), Client::get()->getMsgCount(), Client::get()->getMaxMsgCount());

    ImGui::Text("Framework: Hakkun");
    ImGui::Text("Mod version: %s\n", TOSTRING(BUILDVERSTR));
    ImGui::Text("Server is running version: %s\n", Client::getServerVersion());

    static bool showSettingsWindow = false;

    // ===== AUTHORIZED USER ONLY CONTENT =====
    if (!isAuthorizedUser) {
        if (gmm->getMode<GameModeBase>()) {
            ImGui::Text("\n------------------- Controls --------------------\n");
            gmm->getMode<GameModeBase>()->debugMenuControls();
        }
        ImGui::End();
        return;
    }
    if (showSettingsWindow) {
        SmooSettings::showSmooSettingsWindow(&showSettingsWindow);
    }

    if (ImGui::Button("SMOO+ Settings")) {
        showSettingsWindow = true;
    }

    // ===== 3D DEBUG RENDERING (Authorized users only) =====
    if (curScene && isInGame) {
        sead::LookAtCamera* cam = &const_cast<sead::LookAtCamera&>(al::getLookAtCamera(curScene, 0));
        sead::Projection* projection = cam ? &const_cast<sead::Projection&>(al::getProjectionSead(curScene, 0)) : nullptr;

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

            GameMode gameMode = gmm->getGameMode();
            GameModeBase* gameModeBase = gmm->getMode<GameModeBase>();

            ImGui::Text("(ZR ←)------------ Page %d/%d -------------(ZR →)\n", pageIndex + 1, maxPages);

            switch (pageIndex) {
            case 0: {
                ImGui::Text("(ZL ←)----------%s Player %d/%d %s-----------(ZL →)\n\n", debugPuppetIndex + 1 < 10 ? "-" : "", debugPuppetIndex + 1,
                            Client::getMaxPlayerCount(), Client::getMaxPlayerCount() < 10 ? "-" : "");

                if (debugPuppetIndex == 0) {
                    ImGui::Text("Player Name: %s\n", Client::getClientName());
                    ImGui::Text("Connection Status: %s\n", isConnected ? "Online" : "Offline");
                    ImGui::Text("Game mode: %i | %s\n", gameMode, GameModeFactory::getModeName(gameMode));
                    ImGui::Text("Is in same Stage: Yes\n");
                    ImGui::Text("Stage: %s\n", client->getLastGameInfPacket()->stageName);
                    ImGui::Text("Scenario: %u\n", client->getLastGameInfPacket()->scenarioNo);
                    ImGui::Text("Costume: H: %s B: %s\n", client->getLastCostumeInfPacket()->capModel, client->getLastCostumeInfPacket()->bodyModel);
                    ImGui::Text("Capture: %s\n", client->getLastCaptureInfPacket()->hackName);

                    PlayerHackKeeper* hackKeeper = playerBase->getPlayerHackKeeper();
                    if (hackKeeper) {
                        PlayerActorHakoniwa* p1 = (PlayerActorHakoniwa*)playerBase;
                        if (hackKeeper->mCurrentHackActor && p1 && isInGame && curScene) {
                            ImGui::Text("Animation: %s\n", al::getActionName(hackKeeper->mCurrentHackActor));
                        } else {
                            ImGui::Text("Animation: %s\n", p1->mAnimator->mAnimFrameCtrl->getActionName());
                        }
                    }
                } else if (curPuppet) {
                    al::LiveActor* curModel = curPuppet->getCurrentModel();
                    PuppetInfo* curPupInfo = curPuppet->getInfo();

                    if (curModel && curPupInfo) {
                        ImGui::Text("Player Name: %s\n", curPupInfo->puppetName);
                        ImGui::Text("Connection Status: %s\n", curPupInfo->isConnected ? "Online" : "Offline");
                        GameMode puppetGameMode = static_cast<GameMode>(curPupInfo->gameMode);
                        ImGui::Text("Game mode: %i | %s\n", curPupInfo->gameMode, GameModeFactory::getModeName(puppetGameMode));
                        ImGui::Text("Is in same Stage: %s\n", curPupInfo->isInSameStage ? "Yes" : "No");
                        ImGui::Text("Stage: %s\n", curPupInfo->stageName);
                        ImGui::Text("Scenario: %u\n", curPupInfo->scenarioNo);
                        ImGui::Text("Costume: H: %s B: %s\n", curPupInfo->costumeHead, curPupInfo->costumeBody);
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

                    if (hackKeeper->mCurrentHackActor) {
                        al::LiveActor* curHack = hackKeeper->mCurrentHackActor;
                        ImGui::Text("Current Hack Animation: %s\n", al::getActionName(curHack));
                        ImGui::Text("Current Hack Name: %s\n", hackKeeper->getCurrentHackName());

                        sead::Quatf captureRot = curHack->mPoseKeeper->getQuat();
                        ImGui::Text("Current Hack Rot: %.3f %.3f %.3f %f\n", captureRot.x, captureRot.y, captureRot.z, captureRot.w);

                        sead::Quatf calcRot;
                        al::calcQuat(&calcRot, curHack);
                        ImGui::Text("Calc Hack Rot: %.3f %.3f %.3f %.3f\n", calcRot.x, calcRot.y, calcRot.z, calcRot.w);
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
                                        capTrans->x, capRot->x, capTrans->y, capRot->y, capTrans->z, capRot->z);
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

                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    pos.x -= 10;
                    ImVec2 size(300, 24);

                    f32 const progress = 1.0f - static_cast<float>(heap->getFreeSize()) / heap->getSize();

                    ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), 0xFF966C52);  // fill
                    ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x * progress, pos.y + size.y),
                                                              0xFF13869D);  // fill

                    float used = isKB ? (heap->getSize() - heap->getFreeSize()) / 1024.f : (heap->getSize() - heap->getFreeSize()) / (1024.f * 1024.f);
                    float max = isKB ? heap->getSize() / 1024.f : heap->getSize() / (1024.f * 1024.f);
                    float percentUsed = (heap->getSize() - heap->getFreeSize()) / (float(heap->getSize()) / 100);
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.3f/%.3f %s", used, max, isKB ? "KB" : "MB");

                    ImGui::ProgressBar(percentUsed / 100, ImVec2(-FLT_MIN, 0), buf);
                };

                displayHeapInfo(Client::getClientHeap(), "Client", true);
                displayHeapInfo(GameModeManager::sInstance->getHeap(), "GameMode", true);
                displayHeapInfo(imgui::sImGuiHeap, "ImGui");
                displayHeapInfo(al::getStationedHeap(), "Stationed");
                displayHeapInfo(al::getSequenceHeap(), "Sequence");
                displayHeapInfo(al::getSceneHeap(), "Scene");
                displayHeapInfo(al::getSceneResourceHeap(), "SceneResource", true);
                displayHeapInfo(al::getWorldResourceHeap(), "WorldResource");

                break;
            }
            case 3: {
                ImGui::Text("------------------- Controls --------------------\n\n");

                if (gameModeBase) {
                    gameModeBase->debugMenuControls();
                }
                ImGui::Text("\n- ZR + ↑ | Open/close this debug menu\n");
                break;
            }
            default:
                break;
            }

            // Render 3D debug spheres
            renderer->begin();
            renderer->setModelMatrix(sead::Matrix34f::ident);

            if (curPuppet) {
                renderer->drawSphere4x8(curPuppet->getInfo()->playerPos, 20, sead::Color4f(1.f, 0.f, 0.f, 0.25f));
                renderer->drawSphere4x8(al::getTrans(curPuppet), 20, sead::Color4f(0.f, 0.f, 1.f, 0.25f));
            } else if (debugPuppetIndex == 0) {
                renderer->drawSphere4x8(client->getLastPlayerInfPacket()->playerPos, 20, sead::Color4f(1.f, 0.f, 0.f, 0.25f));
            }

            renderer->end();
        }
        isInGame = false;
        ImGui::End();
        return;
    }

    if (gmm->getMode<GameModeBase>()) {
        ImGui::Text("\n------------------- Controls --------------------\n");
        gmm->getMode<GameModeBase>()->debugMenuControls();
    }
    ImGui::End();
}

// ===== LOGGING HOOK =====

void seadPrintHook(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    Logger::log(fmt, args);

    va_end(args);
}

extern "C" void hkMain() {
    // Init Stuff
    gameSystemInit.installAtSym<"_ZN10GameSystem4initEv">();
    hakoniwaSequenceInitHook.installAtSym<"_ZN16HakoniwaSequence4initERKN2al16SequenceInitInfoE">();
    insertCustomThingsInFactory();
    drawTableHook.installAtSym<"_ZN2al15ExecuteDirector4initERKNS_21ExecuteSystemInitInfoE">();

    initActorInitInfoHook.installAtSym<"R_ZN2al17initActorInitInfo">();

    // Debug Stuff
    drawMainHookHk.installAtSym<"_ZN10GameSystem8drawMainEv">();
    hk::hook::writeBranchAtSym<"_ZN4sead6system5PrintEPKcz">(seadPrintHook);

    // Main Stuff
    hakoniwaSequenceHook.installAtSym<"_ZN16HakoniwaSequence12exePlayStageEv">();
    initMarioModelActorHook.installAtSym<"R_ZN14PlayerFunction19initMarioModelActor">();

    hk::hook::trampoline([]() -> void {}).installAtSym<"_ZN2rs21requestShowHtmlViewerEPKN2al18IUseSceneObjHolderE">();  // Disable Action Guide / HtmlViewer
    disableAppearSwitchCameraHook.installAtSym<"R_ZN17AppearSwitchTimer4init">();  // disables AppearSwitchTimer's camera switch
    // Puppet Actor Setup
    initPuppetActorsHook.installAtSym<"_ZN2al22initPlacementObjectMapEPNS_5SceneERKNS_13ActorInitInfoEPKc">();

    // Shine Syncing
    sendShinePacketHook.installAtSym<"_ZN16GameDataFunction11setGotShineE20GameDataHolderWriterPK9ShineInfo">();
    sendShinePacketHook2.installAtSym<"_ZN12GameDataFile14getAchievementEPKc">();
    registerShineToListHook.installAtSym<"_ZN5Shine18initAfterPlacementEv">();

    // CoinCollect Syncing
    sendCoinCollectCollectPacketHook.installAtSym<"_ZN12GameDataFile14addCoinCollectEPKN2al11PlacementIdE">();
    registerCoinCollectToListHook.installAtSym<"_ZN11CoinCollect18initAfterPlacementEv">();
    registerCoinCollect2DToListHook.installAtSym<"_ZN13CoinCollect2D18initAfterPlacementEv">();

    // Amiibo Button Disabling
    hk::hook::replace([]() -> void { return; }).installAtSym<"_ZN2rs16isHoldAmiiboModeEPKN2al18IUseSceneObjHolderE">();
    hk::hook::replace([]() -> void { return; }).installAtSym<"_ZN2rs19isTriggerAmiiboModeEPKN2al18IUseSceneObjHolderE">();

    // Remap Snapshot to !L + Down, disables snapshot mode in freeze tag
    comboBtnHook.installAtSym<"_ZN2rs21isTriggerSnapShotModeEPKN2al18IUseSceneObjHolderE">();

    // Capture Syncing
    initObjHook.installAtSym<"_ZN2al31createPlacementActorFromFactoryERKNS_13ActorInitInfoEPKNS_13PlacementInfoE">();

    // Save Data Edits
    saveWriteHook.installAtSym<"_ZN14GameConfigData5writeEPN2al11ByamlWriterE">();
    saveReadHook.installAtSym<"_ZN14GameConfigData4readERKN2al9ByamlIterE">();

    // WindowConfirm Edits (Forces logic to ignore current nerve)
    windowConfirmWaitHook.installAtSym<"_ZN2al17WindowConfirmWait6tryEndEv">();

    // Coin Counter Changes
    startCoinCounterHook.installAtSym<"_ZN11CoinCounter8tryStartEv">();

    // Other HUD Changes
    hk::hook::writeBranchLinkAtMainOffset(0x20cb4c, modeE3Hook);  // PlayGuideMenuLyt at StageSceneStateLayout::start+140
    hk::hook::writeBranchLinkAtMainOffset(0x20ca5c, modeE3Hook);  // MapMini::appearSlideIn at StageSceneStateLayout::start+50
    hk::hook::writeBranchLinkAtMainOffset(0x20d160, modeE3Hook);  // MapMini::end at StageSceneStateLayout::exeEnd+8C
    hk::hook::writeBranchLinkAtMainOffset(0x20d154, playGuideEndHook);

    // Pause Menu Changes

    hk::hook::a64::assemble<"mov w2, #5">().installAtSym<"R_ZN24StageSceneStatePauseMenuNrvStateCount">();  // increase nerve state count to 5
    initNerveStateHook.installAtSym<"R_ZN24StageSceneStatePauseMenuC1">();                                  // inits options nerve state and server config state
    pauseMenuWaitHook.installAtSym<"_ZN24StageSceneStatePauseMenu7exeWaitEv">();                            // Change Action Guide Text + Onine Indicator

    // inits StageSceneStateOption and StageSceneStateServerConfig
    initStateHook.installAtSym<"_ZN21StageSceneStateOptionC1EPKcPN2al5SceneERKNS2_14LayoutInitInfoEP11FooterPartsP14GameDataHolderb">();
    overrideHelpFadeNerve.installAtSym<"_ZN24StageSceneStatePauseMenu17exeFadeBeforeHelpEv">();

    // Gravity hooks
    hk::hook::trampoline([]() -> void { return; }).installAtSym<"_ZN28PlayerJointControlGroundPose6updateEffffb">();
    hk::hook::writeBranchLinkAtSym<"R_hackCapTQGSV">(initHackCapHook);
    stageSceneInitHook.installAtSym<"_ZN10StageScene4initERKN2al13SceneInitInfoE">();  // create custom gravity camera ticket
    borderPullBackHook.installAtSym<"_ZN20WorldEndBorderKeeper11exePullBackEv">();     // hooks WorldEndBorderKeeper to kill the
                                                                                       // player if they reach the map border

    // Freeze tag hooks
    isCheckpointWarpAllowedHook.installAtSym<"_ZNK9MapLayout22isEnableCheckpointWarpEv">();  // always allow warping except in freeze tag
    freezeDeathAreaHook.installAtSym<"_ZN2al13isInDeathAreaEPKNS_9LiveActorE">();            // Replaces functionality of death areas in freeze tag
    playerHitPointDamageHook.installAtSym<"_ZN18PlayerHitPointData6damageEv">();             // disables the damage function in Freeze Tag
    isEnableRescuePlayerHook.installAtSym<"_ZNK7HackCap20isEnableRescuePlayerEv">();         // Forces kids mode to be enabled during Freeze Tag
    freezeMoonHitboxHook.installAtSym<"_ZN5Shine14makeActorAliveEv">();                      // When mode enabled, disable moon
                                                                                             // hitboxes to avoid softlocks

    // custom bootscreen hooks
    hk::hook::writeBranchLinkAtSym<"R_hakoniwaSetNerveSetup">(speedboot::hakoniwaSetNerveSetup);
    hk::hook::a64::assemble<"mov w2, #0x1f">().installAtSym<"R_hakoniwaSetNerveCount">();  // nerve state count
    speedboot::prepareSpeedBootHook.installAtSym<"_ZN10BootLayoutC1ERKN2al14LayoutInitInfoE">();

    // unlock costume doors
    unlockCostumeDoorsHook.installAtSym<"_ZN2al19listenStageSwitchOnEPNS_15IUseStageSwitchEPKcRKNS_11FunctorBaseE">();  // all except metro
    hk::hook::writeBranchLinkAtSym<"R_metroCostumeDoor">(unlockCostumeDoorMetroHook);                                   // metro

    // QOL Patches
    // hk::hook::a64::assemble<"nop">().installAtMainOffset(0x4DB934);  // LifeUpMaxItem demo skip
    // hk::hook::a64::assemble<"nop">().installAtMainOffset(0x2D250C);  // Notes Demo Skip
    // hk::hook::a64::assemble<"nop">().installAtMainOffset(0x45c69c);  // Removes Assist Mode Ledge Grabs

    // World Resource Heap stuff
    hk::ro::getMainModule()->writeRo(0x5145c8, 0x7107D29F);  // cmp w20, #500
    hk::hook::a64::assemble<"ret">().installAtMainOffset(0x514710);

    // Twists
    icePhysicsHook.installAtSym<"_ZN2al11isFloorCodeERKNS_8TriangleEPKc">();  // Enables Ice Physics

    hk::gfx::ImGuiBackendNvn::instance()->installHooks(false);
    hk::gfx::DebugRenderer::instance()->installHooks();
}
