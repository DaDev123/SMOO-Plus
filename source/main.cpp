/**
 * @file main.cpp
 * @brief Core game hooks and main functionality for the multiplayer client
 */

#include "main.hpp"

// ===== SYSTEM INCLUDES =====
#include <cmath>
#include <math.h>

// ===== AL/GAME ENGINE INCLUDES =====
#include "al/execute/ExecuteOrder.h"
#include "al/execute/ExecuteTable.h"
#include "al/execute/ExecuteTableHolderDraw.h"
#include "al/LiveActor/LiveActor.h"
#include "al/util.hpp"
#include "al/util/AudioUtil.h"
#include "al/util/CameraUtil.h"
#include "al/util/ControllerUtil.h"
#include "al/util/GraphicsUtil.h"
#include "al/util/LiveActorUtil.h"
#include "al/util/NerveUtil.h"

// ===== GAME INCLUDES =====
#include "game/GameData/GameDataFunction.h"
#include "game/GameData/GameDataHolderAccessor.h"
#include "game/HakoniwaSequence/HakoniwaSequence.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Player/PlayerFunction.h"
#include "game/Player/PlayerHackKeeper.h"
#include "game/StageScene/StageScene.h"

// ===== SEAD INCLUDES =====
#include "container/seadSafeArray.h"
#include "heap/seadHeap.h"
#include "math/seadVector.h"

// ===== PROJECT INCLUDES =====
#include "actors/PuppetActor.h"
#include "debugMenu.hpp"
#include "helpers.hpp"
#include "layouts/HideAndSeekIcon.h"
#include "logger.hpp"
#include "puppets/PuppetInfo.h"
#include "rs/util.hpp"
#include "server/Client.hpp"
#include "server/freeze/FreezeTagMode.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "server/hns/HideAndSeekMode.hpp"

// ===== GLOBAL VARIABLES =====
static int pInfSendTimer = 0;
static int gameInfSendTimer = 0;
static int chatUpdateTimer = 0;
static int debugPuppetIndex = 0;
static int debugCaptureIndex = 0;
static int pageIndex = 0;
static const int maxPages = 3;

al::SequenceInitInfo* initInfo;

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
            PlayerHitPointData* data = holder.mData->mGameDataFile->getPlayerHitPointData();
            data->mIsKidsMode = Client::shouldKids();
            data->mCurrentHit = Client::getHealth();
            Client::setNeedUpdateHealthCoins(false);
        }

        gameInfSendTimer = 0;
    }

    // In updatePlayerInfo function, replace the chat timer section:

if (chatUpdateTimer >= 450)  
{
    // Shift messages up and clear the oldest
    if (!Client::getMessage(0).isEmpty())
    {
        Client::setMessage(0, Client::getMessage(1).cstr());
        Client::setMessage(1, Client::getMessage(2).cstr());
        Client::setMessage(2, "");
        chatUpdateTimer = 0;
    }
}
else if (!Client::getMessage(0).isEmpty() || !Client::getMessage(1).isEmpty() ||
    !Client::getMessage(2).isEmpty())
{
    chatUpdateTimer++;
}

    
    pInfSendTimer++;
    gameInfSendTimer++;
}

// ===== MAIN DRAW HOOK =====

void drawMainHook(HakoniwaSequence* curSequence, sead::Viewport* viewport, sead::DrawContext* drawContext) {
    GameModeManager* gmm = GameModeManager::instance();
    GameModeBase* mode = gmm->getMode<GameModeBase>();

    // Freeze tag needs the delta time to not update while the game is paused
    if (gmm->isMode(GameMode::FREEZETAG)) {
        if (!gmm->isPaused())
            Time::calcTime();
    } else {
        Time::calcTime();
    }

    // Setup text writer
    int dispHeight = al::getLayoutDisplayHeight();
    gTextWriter->mViewport = viewport;
    gTextWriter->mColor = sead::Color4f(1.f, 1.f, 1.f, 0.8f);

    // Get scene and basic info
    al::Scene* curScene = curSequence->curScene;
    Client* client = Client::instance();
    SocketClient* socket = client->mSocket;
    bool isConnected = socket->isConnected();
    bool isPaused = gmm->isPaused();

    // Check authorization
    const char* currentUser = Client::getClientName();
    bool isAuthorizedUser = (strcmp(currentUser, "SrDev") == 0) || (strcmp(currentUser, "Crafty") == 0) || (strcmp(currentUser, "KleinTimmi") == 0);

// ===== PAUSE MENU DEBUG WINDOW =====
// Check using the GameModeManager's pause state
if (gmm->isPaused()) {
    // Draw background on the right side
    drawConnectionBackground((agl::DrawContext*)drawContext);
    
    gTextWriter->beginDraw();
    
    // Get display dimensions
    int dispWidth = al::getLayoutDisplayWidth();
    
    // Calculate position for top-right
    float rightPadding = 10.f;
    float topPadding = 5.f;
    float textWidth = 200.f;
    float xPos = dispWidth - textWidth - rightPadding;
    float yPos = topPadding;
    
    gTextWriter->setScaleFromFontHeight(13.f);
    
    // Cyan header
    gTextWriter->setCursorFromTopLeft(sead::Vector2f(xPos, yPos));
    gTextWriter->mColor = sead::Color4f(0.f, 1.f, 1.f, 1.f);
    gTextWriter->printf("======= CONNECTION INFO =======\n\n");
    
    // Server IP and Port
    gTextWriter->setCursorFromTopLeft(sead::Vector2f(xPos, yPos + 30.f));
    if (Client::isServerHidden()) {
        gTextWriter->mColor = sead::Color4f(1.f, 1.f, 0.f, 1.f); // Yellow label
        gTextWriter->printf("Server: ");
        gTextWriter->mColor = sead::Color4f(0.5f, 0.5f, 0.5f, 1.f); // Gray
        gTextWriter->printf("<hidden>");
    } else {
        gTextWriter->mColor = sead::Color4f(1.f, 1.f, 0.f, 1.f); // Yellow label
        gTextWriter->printf("Server: ");
        gTextWriter->mColor = sead::Color4f(1.f, 1.f, 1.f, 1.f); // White
        gTextWriter->printf("%s:%d", socket->getIP(), socket->getPort());
    }
    
    // Connection status
    gTextWriter->setCursorFromTopLeft(sead::Vector2f(xPos, yPos + 45.f));
    gTextWriter->mColor = sead::Color4f(1.f, 1.f, 0.f, 1.f); // Yellow label
    gTextWriter->printf("Status: ");
    if (isConnected) {
        gTextWriter->mColor = sead::Color4f(0.f, 1.f, 0.f, 1.f); // Green
        gTextWriter->printf("Connected");
    } else {
        gTextWriter->mColor = sead::Color4f(1.f, 0.f, 0.f, 1.f); // Red
        gTextWriter->printf("Disconnected");
    }
    
    // Player count
    gTextWriter->setCursorFromTopLeft(sead::Vector2f(xPos, yPos + 60.f));
    gTextWriter->mColor = sead::Color4f(1.f, 1.f, 0.f, 1.f); // Yellow label
    gTextWriter->printf("Players: ");
    if (isConnected) {
        gTextWriter->mColor = sead::Color4f(1.f, 1.f, 1.f, 1.f); // White
        gTextWriter->printf("%d/%d", 
            Client::getConnectCount() + 1, 
            Client::getMaxPlayerCount());
    } else {
        gTextWriter->mColor = sead::Color4f(0.5f, 0.5f, 0.5f, 1.f); // Gray
        gTextWriter->printf("N/A");
    }
    
    gTextWriter->endDraw();
    
    al::executeDraw(curSequence->mLytKit, "２Ｄバック（メイン画面）");
    return;
}

    // ===== CHAT RENDERING (Non-debug mode, in-game only) =====
    if (!debugMode && curScene && isInGame) {
        // Try to get camera for chat rendering
        sead::LookAtCamera* cam = al::getLookAtCamera(curScene, 0);
        sead::Projection* projection = cam ? al::getProjectionSead(curScene, 0) : nullptr;

        if (cam && projection) {
            sead::PrimitiveRenderer* renderer = sead::PrimitiveRenderer::instance();
            renderer->setDrawContext(drawContext);
            renderer->setCamera(*cam);
            renderer->setProjection(*projection);

            int msgCount = 0;
            for (int i = 0; i < 3; i++) {
                if (!Client::getMessage(i).isEmpty()) msgCount++;
            }

            if (msgCount > 0) {
                drawChatBackground((agl::DrawContext*)drawContext, (float)(4 - msgCount));
                
                gTextWriter->beginDraw();
                gTextWriter->setScaleFromFontHeight(15.f);
                
                float baseY = (dispHeight * 7 / 10) + 95.f - 5.f;
                float lineHeight = 18.f;
                
                // Draw messages from oldest to newest (bottom to top)
                for (int i = msgCount - 1; i >= 0; i--) {
                    if (!Client::getMessage(i).isEmpty()) {
                        float yPos = baseY - (lineHeight * (msgCount - 1 - i));
                        gTextWriter->setCursorFromTopLeft(sead::Vector2f(10.f, yPos));
                        gTextWriter->printf("%s\n", Client::getMessage(i).cstr());
                    }
                }
                
                gTextWriter->endDraw();
            }
        }

		isInGame = false;
        al::executeDraw(curSequence->mLytKit, "２Ｄバック（メイン画面）");
        return;
    }

    // ===== NON-DEBUG MODE EXIT =====
    if (!debugMode) {
        al::executeDraw(curSequence->mLytKit, "２Ｄバック（メイン画面）");
        return;
    }

    // ===== DEBUG MODE RENDERING =====
    drawBackground((agl::DrawContext*)drawContext);

    gTextWriter->beginDraw();
    gTextWriter->setCursorFromTopLeft(sead::Vector2f(10.f, 10.f));

    gTextWriter->printf("FPS: %d\n", static_cast<int>(round(Application::sInstance->mFramework->calcFps())));

    gTextWriter->setCursorFromTopLeft(sead::Vector2f(10.f, (dispHeight / 3) + 30.f));
    gTextWriter->setScaleFromFontHeight(20.f);

    // Server info
    if (Client::isServerHidden()) {
        gTextWriter->printf(
            isConnected ? "Server: <hidden> | %d/%d Players\n" : "Server: <hidden>\n",
            isConnected ? Client::getConnectCount() + 1 : 0,
            isConnected ? Client::getMaxPlayerCount() : 0
        );
    } else {
        gTextWriter->printf(
            isConnected ? "Server: %s:%d | %d/%d Players\n" : "Server: %s:%d\n",
            socket->getIP(),
            socket->getPort(),
            isConnected ? Client::getConnectCount() + 1 : 0,
            isConnected ? Client::getMaxPlayerCount() : 0
        );
    }
    gTextWriter->printf("Your TCP status: %s\n", socket->getStateChar());

    // Heap info
    sead::Heap* clientHeap = Client::getClientHeap();
    if (clientHeap) {
        sead::Heap* gmHeap = gmm->getHeap();
        if (gmHeap && clientHeap->getSize() > 0 && gmHeap->getSize() > 0) {
            size_t clientUsed = clientHeap->getSize() - clientHeap->getFreeSize();
            size_t clientTotal = clientHeap->getSize();
            size_t gmUsed = gmHeap->getSize() - gmHeap->getFreeSize();
            size_t gmTotal = gmHeap->getSize();
        
            gTextWriter->printf(
                "Heap Use: %.1f/%.0f (Client) %.1f/%.0f (Gmode)\n",
                0.0009765625 * clientUsed,
                0.0009765625 * clientTotal,
                0.0009765625 * gmUsed,
                0.0009765625 * gmTotal
            );
        } else {
            gTextWriter->printf("Heap Use: Invalid heap sizes\n");
        }
    } else {
        gTextWriter->printf("Heap Use: Client heap unavailable\n");
    }

    // Queue info
    gTextWriter->printf(
        "Queue Count: %d/%d (Send) %d/%d (Receive)\n",
        socket->getSendCount(),
        socket->getSendMaxCount(),
        socket->getRecvCount(),
        socket->getRecvMaxCount()
    );

    gTextWriter->printf("Mod version: %s\n", TOSTRING(BUILDVERSTR));
    gTextWriter->printf("Server is running version: %s\n", Client::getServerVersion());


    // ===== AUTHORIZED USER ONLY CONTENT =====
    if (!isAuthorizedUser) {
        gTextWriter->endDraw();
        al::executeDraw(curSequence->mLytKit, "２Ｄバック（メイン画面）");
        return;
    }

    // ===== 3D DEBUG RENDERING (Authorized users only) =====
    if (curScene && isInGame) {
        sead::LookAtCamera* cam = al::getLookAtCamera(curScene, 0);
        sead::Projection* projection = cam ? al::getProjectionSead(curScene, 0) : nullptr;

        if (cam && projection) {
            PlayerActorBase* playerBase = rs::getPlayerActor(curScene);
            PuppetActor* curPuppet = Client::getPuppet(debugPuppetIndex - 1);
            PuppetActor* debugPuppet = Client::getDebugPuppet();
            
            if (debugPuppet) {
                curPuppet = debugPuppet;
            }

            sead::PrimitiveRenderer* renderer = sead::PrimitiveRenderer::instance();
            renderer->setDrawContext(drawContext);
            renderer->setCamera(*cam);
            renderer->setProjection(*projection);

            GameMode gameMode = gmm->getGameMode();

            gTextWriter->printf("(ZR ←)------------ Page %d/%d -------------(ZR →)\n", pageIndex + 1, maxPages);

            switch (pageIndex) {
            case 0: {
                gTextWriter->printf(
                    "(ZL ←)----------%s Player %d/%d %s-----------(ZL →)\n\n",
                    debugPuppetIndex + 1 < 10 ? "-" : "",
                    debugPuppetIndex + 1,
                    Client::getMaxPlayerCount(),
                    Client::getMaxPlayerCount() < 10 ? "-" : ""
                );

                if (debugPuppetIndex == 0) {
                    gTextWriter->printf("Player Name: %s\n", Client::getClientName());
                    gTextWriter->printf("Connection Status: %s\n", isConnected ? "Online" : "Offline");
                    gTextWriter->printf("Game mode: %i | %s\n", gameMode, GameModeFactory::getModeName(gameMode));
                    gTextWriter->printf("Is in same Stage: Yes\n");
                    gTextWriter->printf("Stage: %s\n", client->getLastGameInfPacket()->stageName);
                    gTextWriter->printf("Scenario: %u\n", client->getLastGameInfPacket()->scenarioNo);
                    gTextWriter->printf("Costume: H: %s B: %s\n", client->getLastCostumeInfPacket()->capModel, client->getLastCostumeInfPacket()->bodyModel);
                    gTextWriter->printf("Capture: %s\n", client->getLastCaptureInfPacket()->hackName);

                    PlayerHackKeeper* hackKeeper = playerBase->getPlayerHackKeeper();
                    if (hackKeeper) {
                        PlayerActorHakoniwa* p1 = (PlayerActorHakoniwa*)playerBase;
                        if (hackKeeper->currentHackActor) {
                            gTextWriter->printf("Animation: %s\n", al::getActionName(hackKeeper->currentHackActor));
                        } else {
                            gTextWriter->printf("Animation: %s\n", p1->mPlayerAnimator->mAnimFrameCtrl->getActionName());
                        }
                    }
                } else if (curPuppet) {
                    al::LiveActor* curModel = curPuppet->getCurrentModel();
                    PuppetInfo* curPupInfo = curPuppet->getInfo();

                    if (curModel && curPupInfo) {
                        gTextWriter->printf("Player Name: %s\n", curPupInfo->puppetName);
                        gTextWriter->printf("Connection Status: %s\n", curPupInfo->isConnected ? "Online" : "Offline");
                        gTextWriter->printf("Is in same Stage: %s\n", curPupInfo->isInSameStage ? "Yes" : "No");
                        gTextWriter->printf("Stage: %s\n", curPupInfo->stageName);
                        gTextWriter->printf("Scenario: %u\n", curPupInfo->scenarioNo);
                        gTextWriter->printf("Costume: H: %s B: %s\n", curPupInfo->costumeHead, curPupInfo->costumeBody);
                        gTextWriter->printf("Capture: %s\n", curPupInfo->isCaptured ? curPupInfo->curHack : "");
                        gTextWriter->printf("Animation: %d %s\n", curPupInfo->curAnim, curPupInfo->curAnimStr);
                        
                        const char* modelAnim = al::getActionName(curModel);
                        gTextWriter->printf("Model Animation: %s\n", modelAnim ? modelAnim : "none");
                        
                        if (curModel->mActorActionKeeper) {
                            gTextWriter->printf("Is Action Playing: %s\n", 
                                al::isActionPlaying(curModel, curPupInfo->curAnimStr) ? "Yes" : "No");
                            gTextWriter->printf("Is Action End: %s\n", 
                                al::isActionEnd(curModel) ? "Yes" : "No");
                            gTextWriter->printf("Is Capture Model: %s\n", 
                                curPupInfo->isCaptured ? "Yes" : "No");
                        }
                    }
                }
                break;
            }
            case 1: {
                gTextWriter->printf("--------------- Animation & Cappy ---------------\n\n");
                PlayerHackKeeper* hackKeeper = playerBase->getPlayerHackKeeper();

                if (hackKeeper) {
                    PlayerActorHakoniwa* p1 = (PlayerActorHakoniwa*)playerBase;

                    if (hackKeeper->currentHackActor) {
                        al::LiveActor* curHack = hackKeeper->currentHackActor;
                        gTextWriter->printf("Current Hack Animation: %s\n", al::getActionName(curHack));
                        gTextWriter->printf("Current Hack Name: %s\n", hackKeeper->getCurrentHackName());
                        
                        sead::Quatf captureRot = curHack->mPoseKeeper->getQuat();
                        gTextWriter->printf("Current Hack Rot: %.3f %.3f %.3f %f\n", captureRot.x, captureRot.y, captureRot.z, captureRot.w);
                        
                        sead::Quatf calcRot;
                        al::calcQuat(&calcRot, curHack);
                        gTextWriter->printf("Calc Hack Rot: %.3f %.3f %.3f %.3f\n", calcRot.x, calcRot.y, calcRot.z, calcRot.w);
                    } else {
                        gTextWriter->printf("Cur Action: %s\n", p1->mPlayerAnimator->mAnimFrameCtrl->getActionName());
                        gTextWriter->printf("Cur Anim: %s\n", p1->mPlayerAnimator->curAnim.cstr());
                        gTextWriter->printf("Cur Sub Anim: %s\n", p1->mPlayerAnimator->curSubAnim.cstr());
                        gTextWriter->printf("Is Cappy Flying? %s\n", BTOC(p1->mHackCap->isFlying()));
                        
                        if (p1->mHackCap->isFlying()) {
                            gTextWriter->printf("Cappy Action: %s\n", al::getActionName(p1->mHackCap));
                            sead::Vector3f* capTrans = al::getTransPtr(p1->mHackCap);
                            sead::Vector3f* capRot = &p1->mHackCap->mJointKeeper->mJointRot;
                            gTextWriter->printf("Cappy: Position   Rotation\nX:   % 10.3f % 10.3f\nY:   % 10.3f % 10.3f\nZ:   % 10.3f % 10.3f\n",
                                capTrans->x, capRot->x,
                                capTrans->y, capRot->y,
                                capTrans->z, capRot->z
                            );
                            gTextWriter->printf("Cappy Skew: %.3f\n", p1->mHackCap->mJointKeeper->mSkew);
                        }
                    }
                }
                break;
            }
            case 2: {
                gTextWriter->printf("------------------- Controls --------------------\n\n");
                gTextWriter->printf("\n- ZR + ↑ | Open/close this debug menu\n");
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
    }

    gTextWriter->endDraw();
    al::executeDraw(curSequence->mLytKit, "２Ｄバック（メイン画面）");
}
    

// ===== SHINE PACKET FUNCTION =====

void sendShinePacket(GameDataHolderAccessor thisPtr, Shine* curShine) {

    if (!curShine->isGot()) {

        GameDataFile::HintInfo* curHintInfo =
            &thisPtr.mData->mGameDataFile->mShineHintList[curShine->mShineIdx];

        Client::sendShineCollectPacket(curHintInfo->mUniqueID);
    }

    GameDataFunction::setGotShine(thisPtr, curShine->curShineInfo);
}

// ===== STAGE INITIALIZATION HOOK =====

void stageInitHook(al::ActorInitInfo *info, StageScene *curScene, al::PlacementInfo const *placement, al::LayoutInitInfo const *lytInfo, al::ActorFactory const *factory, al::SceneMsgCtrl *sceneMsgCtrl, al::GameDataHolderBase *dataHolder) {

    al::initActorInitInfo(info, curScene, placement, lytInfo, factory, sceneMsgCtrl,
                          dataHolder);

    Client::clearArrays();

    Client::setSceneInfo(*info, curScene);

    if (GameModeManager::instance()->getGameMode() != NONE) {
        GameModeInitInfo initModeInfo(info, curScene);
        initModeInfo.initServerInfo(GameModeManager::instance()->getGameMode(), Client::getPuppetHolder());

        GameModeManager::instance()->initScene(initModeInfo);
    }

    Client::sendGameInfPacket(info->mActorSceneInfo.mSceneObjHolder);
    TwistsConfig::handleStageInit();

}

// ===== PLAYER MODEL SETUP =====

PlayerCostumeInfo *setPlayerModel(al::LiveActor *player, const al::ActorInitInfo &initInfo, const char *bodyModel, const char *capModel, al::AudioKeeper *keeper, bool isCloset) {
    Client::sendCostumeInfPacket(bodyModel, capModel);
    return PlayerFunction::initMarioModelActor(player, initInfo, bodyModel, capModel, keeper, isCloset);
}

// ===== CONSTRUCTION HOOK =====

ulong constructHook() {  // hook for constructing anything we need to globally be accesible

    __asm("STR X21, [X19,#0x208]"); // stores WorldResourceLoader into HakoniwaSequence

    __asm("MOV %[result], X20"
          : [result] "=r"(
              initInfo));  // Save our scenes init info to a gloabl ptr so we can access it later

    Client::createInstance(al::getCurrentHeap());
    GameModeManager::createInstance(al::getCurrentHeap()); // Create the GameModeManager on the current al heap

    return 0x20;
}

// ===== THREAD INITIALIZATION =====

bool threadInit(HakoniwaSequence *mainSeq) {  // hook for initializing client class

    al::LayoutInitInfo lytInfo = al::LayoutInitInfo();

    al::initLayoutInitInfo(&lytInfo, mainSeq->mLytKit, 0, mainSeq->mAudioDirector, initInfo->mSystemInfo->mLayoutSys, initInfo->mSystemInfo->mMessageSys, initInfo->mSystemInfo->mGamePadSys);

    Client::instance()->init(lytInfo, mainSeq->mGameDataHolder);

    return GameDataFunction::isPlayDemoOpening(mainSeq->mGameDataHolder);
}

// ===== MAIN SEQUENCE HOOK =====

bool hakoniwaSequenceHook(HakoniwaSequence* sequence) {
    StageScene* stageScene = (StageScene*)sequence->curScene;

    static bool isCameraActive = false;

    bool isFirstStep = al::isFirstStep(sequence);

    al::PlayerHolder *pHolder = al::getScenePlayerHolder(stageScene);
    PlayerActorBase* playerBase = al::tryGetPlayerActor(pHolder, 0);
    auto *player = (PlayerActorHakoniwa*)al::tryGetPlayerActor(pHolder, 0);
    
    bool isYukimaru = !playerBase->getPlayerInfo();

    isInGame = !stageScene->isPause();

    GameModeManager::instance()->setPaused(stageScene->isPause());
    Client::setStageInfo(stageScene->mHolder);

    Client::update();

    updatePlayerInfo(stageScene->mHolder, playerBase, isYukimaru);

    TwistsConfig::updateCappyProximity(player, stageScene);

    if (al::isPadHoldZR(-1)) {
        if (al::isPadTriggerUp(-1)) { // ZR + Up => Debug menu
            debugMode = !debugMode;
        }
        if (debugMode) {
            if (al::isPadTriggerLeft(-1)) { // [Debug menu] ZR + Left => Previous page
                pageIndex--;
                if (pageIndex < 0) {
                    pageIndex = maxPages - 1;
                }
            }
            if (al::isPadTriggerRight(-1)) { // [Debug menu] ZR + Right => Next page
                pageIndex++;
                if (pageIndex >= maxPages) {
                    pageIndex = 0;
                }
            }
        }
    } else if (al::isPadHoldZL(-1)) {
        if (debugMode && pageIndex == 0) {
            if (al::isPadTriggerLeft(-1)) { // [Debug menu] ZL + Left => Previous player
                debugPuppetIndex--;
                if (debugPuppetIndex < 0) {
                    debugPuppetIndex = Client::getMaxPlayerCount() - 1;
                }
            }
            if (al::isPadTriggerRight(-1)) { // [Debug menu] ZL + Right => Next player
                debugPuppetIndex++;
                if (debugPuppetIndex >= Client::getMaxPlayerCount()) {
                    debugPuppetIndex = 0;
                }
            }
        }
    } else if (al::isPadHoldL(-1)) {
        if (al::isPadTriggerLeft(-1)) { // L + Left => Activate gamemode
            GameModeManager::instance()->toggleActive();
        }
    }
    if (Client::isMusicDisabled()) {
        if (al::isPlayingBgm(stageScene)) {
            al::stopAllBgm(stageScene, 0);
        }
    }

if(isFirstStep && GameModeManager::instance()->isMode(GameMode::FREEZETAG))
    GameModeManager::instance()->getMode<FreezeTagMode>()->setWipeHolder(sequence->mWipeHolder);

return isFirstStep;

    return isFirstStep;

}

// ===== LOGGING HOOK =====

void seadPrintHook(const char *fmt, ...)
{
    va_list args;
	va_start(args, fmt);

    Logger::log(fmt, args);

    va_end(args);
}
