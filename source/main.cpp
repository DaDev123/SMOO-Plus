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
        
        gameInfSendTimer = 0;
    }

    if (chatUpdateTimer >= 300)
    {
        if (!Client::getMessage(0).isEmpty())
        {
            Client::setMessage(0, "");
        }
        if (!Client::getMessage(1).isEmpty())
        {
            Client::setMessage(0, Client::getMessage(1).cstr());
            Client::setMessage(1, "");
        }
        if (!Client::getMessage(2).isEmpty())
        {
            Client::setMessage(1, Client::getMessage(2).cstr());
            Client::setMessage(2, "");
        }
        chatUpdateTimer = 0;
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
    GameModeManager* gmm  = GameModeManager::instance();
    GameModeBase*    mode = gmm->getMode<GameModeBase>();

    // Freeze tag needs the delta time to not update while the game is paused, so if in Freeze Tag mode, override functionality
    if(GameModeManager::instance()->isMode(GameMode::FREEZETAG)) {
        if(!GameModeManager::instance()->isPaused())
            Time::calcTime();
    } else {
        Time::calcTime();  // this needs to be ran every frame, so running it here works
    }

    if (!debugMode) {
        al::executeDraw(curSequence->mLytKit, "２Ｄバック（メイン画面）");
        return;
    }

    Client*       client      = Client::instance();
    SocketClient* socket      = client->mSocket;
    bool          isConnected = socket->isConnected();

    // Check if current user is authorized for full debug menu
    const char* currentUser = Client::getClientName();
    bool isAuthorizedUser = (strcmp(currentUser, "SrDev") == 0) || (strcmp(currentUser, "Crafty") == 0);

    int dispHeight = al::getLayoutDisplayHeight();

    gTextWriter->mViewport = viewport;

    gTextWriter->mColor = sead::Color4f(1.f, 1.f, 1.f, 0.8f);

    drawBackground((agl::DrawContext*)drawContext);

    gTextWriter->beginDraw();
    gTextWriter->setCursorFromTopLeft(sead::Vector2f(10.f, 10.f));

    gTextWriter->printf("FPS: %d\n", static_cast<int>(round(Application::sInstance->mFramework->calcFps())));

    gTextWriter->setCursorFromTopLeft(sead::Vector2f(10.f, (dispHeight / 3) + 30.f));
    gTextWriter->setScaleFromFontHeight(20.f);

    if (Client::isServerHidden()) {
        gTextWriter->printf(
            isConnected ? "Server: <hidden> | %d/%d Players\n" : "Server: <hidden>\n",
            isConnected ? Client::getConnectCount() + 1 : 0,
            isConnected ? Client::getMaxPlayerCount()   : 0
        );
    } else {
        gTextWriter->printf(
            isConnected ? "Server: %s:%d | %d/%d Players\n" : "Server: %s:%d\n",
            socket->getIP(),
            socket->getPort(),
            isConnected ? Client::getConnectCount() + 1 : 0,
            isConnected ? Client::getMaxPlayerCount()   : 0
        );
    }
    gTextWriter->printf("Your TCP status: %s\n", socket->getStateChar());

    sead::Heap* clientHeap = Client::getClientHeap();
    if (clientHeap) {
    sead::Heap* gmHeap = GameModeManager::instance()->getHeap();
        if (gmHeap) {
            // Validate heaps before using them
            if (clientHeap->getSize() > 0 && gmHeap->getSize() > 0) {
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
        gTextWriter->printf("Heap Use: GameMode heap unavailable\n");
        }
    } else {
        gTextWriter->printf("Heap Use: Client heap unavailable\n");
    }

    gTextWriter->printf(
        "Queue Count: %d/%d (Send) %d/%d (Receive)\n",
        socket->getSendCount(),
        socket->getSendMaxCount(),
        socket->getRecvCount(),
        socket->getRecvMaxCount()
    );

    gTextWriter->printf("Mod version: %s\n", TOSTRING(BUILDVERSTR));

    // Only show detailed debug info for authorized users
    if (!isAuthorizedUser) {
        gTextWriter->endDraw();
        al::executeDraw(curSequence->mLytKit, "２Ｄバック（メイン画面）");
        return;
    }

    al::Scene* curScene = curSequence->curScene;

    if (curScene && isInGame) {
        sead::LookAtCamera* cam        = al::getLookAtCamera(curScene, 0);
        sead::Projection*   projection = al::getProjectionSead(curScene, 0);

        PlayerActorBase* playerBase = rs::getPlayerActor(curScene);

        PuppetActor* curPuppet   = Client::getPuppet(debugPuppetIndex - 1);
        PuppetActor* debugPuppet = Client::getDebugPuppet();
        if (debugPuppet) {
            curPuppet = debugPuppet;
        }

        sead::PrimitiveRenderer* renderer = sead::PrimitiveRenderer::instance();
        renderer->setDrawContext(drawContext);
        renderer->setCamera(*cam);
        renderer->setProjection(*projection);

        GameMode      gameMode     = GameModeManager::instance()->getGameMode();
        GameModeBase* gameModeBase = GameModeManager::instance()->getMode<GameModeBase>();

        if (!(Client::getMessage(0) == Client::getMessage(1) &&
              Client::getMessage(1) == Client::getMessage(2))) {
            if (Client::getMessage(0) == Client::getMessage(1))
                drawChatBackground((agl::DrawContext*)drawContext, 3.f);
            else if (Client::getMessage(1).isEmpty())
                drawChatBackground((agl::DrawContext*)drawContext, 2.f);
            else
                drawChatBackground((agl::DrawContext*)drawContext, 1.f);

            gTextWriter->beginDraw();
            gTextWriter->setCursorFromTopLeft(sead::Vector2f(10.f, (dispHeight * 7 / 10) + 60.f));
            gTextWriter->setScaleFromFontHeight(15.f);

            gTextWriter->printf("%s\n", Client::getMessage(0).cstr());
            gTextWriter->printf("%s\n", Client::getMessage(1).cstr());
            gTextWriter->printf("%s\n", Client::getMessage(2).cstr());
        }

        gTextWriter->printf("(ZR ←)------------ Page %d/%d -------------(ZR →)\n", pageIndex + 1, maxPages);

        switch (pageIndex)
        {
        case 0:
            {
                gTextWriter->printf(
                    "(ZL ←)----------%s Player %d/%d %s-----------(ZL →)\n\n",
                    debugPuppetIndex + 1 < 10 ? "-" : "",
                    debugPuppetIndex + 1,
                    Client::getMaxPlayerCount(),
                    Client::getMaxPlayerCount() < 10 ? "-" : ""
                );

                if (debugPuppetIndex == 0) {
                    gTextWriter->printf("Player Name: %s\n",       Client::getClientName());
                    gTextWriter->printf("Connection Status: %s\n", isConnected ? "Online" : "Offline");
                    gTextWriter->printf("Game mode: %i | %s\n",    gameMode, GameModeFactory::getModeName(gameMode));
                    gTextWriter->printf("Is in same Stage: Yes\n");
                    gTextWriter->printf("Stage: %s\n",            client->getLastGameInfPacket()->stageName);
                    gTextWriter->printf("Scenario: %u\n",         client->getLastGameInfPacket()->scenarioNo);
                    gTextWriter->printf("Costume: H: %s B: %s\n", client->getLastCostumeInfPacket()->capModel, client->getLastCostumeInfPacket()->bodyModel);
                    gTextWriter->printf("Capture: %s\n",          client->getLastCaptureInfPacket()->hackName);

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
                        gTextWriter->printf("Player Name: %s\n",       curPupInfo->puppetName);
                        gTextWriter->printf("Connection Status: %s\n", curPupInfo->isConnected ? "Online" : "Offline");
                        gTextWriter->printf("Is in same Stage: %s\n",  curPupInfo->isInSameStage ? "Yes" : "No");
                        gTextWriter->printf("Stage: %s\n",             curPupInfo->stageName);
                        gTextWriter->printf("Scenario: %u\n",          curPupInfo->scenarioNo);
                        gTextWriter->printf("Costume: H: %s B: %s\n",  curPupInfo->costumeHead, curPupInfo->costumeBody);
                        gTextWriter->printf("Capture: %s\n",           curPupInfo->isCaptured ? curPupInfo->curHack : "");
                        gTextWriter->printf("Animation:  %d  %s\n",    curPupInfo->curAnim, curPupInfo->curAnimStr);
                        
                        // NEW: Show what's actually playing on the model
                        const char* modelAnim = al::getActionName(curModel);
                        gTextWriter->printf("Model Animation: %s\n", modelAnim ? modelAnim : "none");
                        
                        // NEW: Show animation state
                        if (curModel->mActorActionKeeper) {
                            gTextWriter->printf("Is Action Playing: %s\n", 
                                al::isActionPlaying(curModel, curPupInfo->curAnimStr) ? "Yes" : "No");
                            gTextWriter->printf("Is Action End: %s\n", 
                                al::isActionEnd(curModel) ? "Yes" : "No");
                            
                            // Show if it's a capture model
                            gTextWriter->printf("Is Capture Model: %s\n", 
                                curPupInfo->isCaptured ? "Yes" : "No");
                        }
                    }
                }
            }
            break;
        case 1:
            {
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
                        gTextWriter->printf("Anim Frame: %.2f\n", p1->mPlayerAnimator->getAnimFrame());
                        gTextWriter->printf("Anim Frame Max: %.2f\n", p1->mPlayerAnimator->getAnimFrameMax());
                        gTextWriter->printf("Anim Frame Rate: %.2f\n", p1->mPlayerAnimator->getAnimFrameRate());
                        gTextWriter->printf("Sub Anim Frame: %.2f\n", p1->mPlayerAnimator->getSubAnimFrame());
                        gTextWriter->printf("Sub Anim Frame Max: %.2f\n", p1->mPlayerAnimator->getSubAnimFrameMax());
                        gTextWriter->printf("Is Sub Anim End: %s\n", BTOC(p1->mPlayerAnimator->isSubAnimEnd()));
                        gTextWriter->printf("Is Upper Body Anim Attached: %s\n", BTOC(p1->mPlayerAnimator->isUpperBodyAnimAttached()));
                        gTextWriter->printf("Blend Weight [0]: %.2f\n", p1->mPlayerAnimator->getBlendWeight(0));
                        gTextWriter->printf("Blend Weight [1]: %.2f\n", p1->mPlayerAnimator->getBlendWeight(1));
                        gTextWriter->printf("Blend Weight [2]: %.2f\n", p1->mPlayerAnimator->getBlendWeight(2));
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
            }
            break;
        case 2:
            {
                gTextWriter->printf("------------------- Controls --------------------\n\n");
                gTextWriter->printf("\n- ZR + ↑ | Open/close this debug menu\n");
            }
            break;
        default:
            break;
        }

        renderer->begin();
        renderer->setModelMatrix(sead::Matrix34f::ident);

        if (curPuppet) {
            renderer->drawSphere4x8(curPuppet->getInfo()->playerPos, 20, sead::Color4f(1.f, 0.f, 0.f, 0.25f));
            renderer->drawSphere4x8(al::getTrans(curPuppet), 20, sead::Color4f(0.f, 0.f, 1.f, 0.25f));
        } else if (debugPuppetIndex == 0) {
            renderer->drawSphere4x8(client->getLastPlayerInfPacket()->playerPos, 20, sead::Color4f(1.f, 0.f, 0.f, 0.25f));
        }

        renderer->end();

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