#include <sys/types.h>
#include "al/execute/ExecuteDirector.h"
#include "al/execute/ExecuteOrder.h"
#include "al/execute/ExecuteTable.h"
#include "al/execute/ExecuteTableHolderDraw.h"
#include "al/execute/ExecuteTableHolderUpdate.h"
#include "al/scene/Scene.h"
#include "al/util/GraphicsUtil.h"
#include "al/util/KitUtil.h"
#include "basis/seadNew.h"
#include "logger.hpp"
#include "rs/util.hpp"
#include "server/Client.hpp"
#include "al/byaml/ByamlIter.h"
#include "al/util.hpp"
#include "game/Actors/WorldEndBorderKeeper.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/StageScene/StageSceneStateOption.h"
#include "game/StageScene/StageSceneStatePauseMenu.h"
#include "game/StageScene/StageSceneStateServerConfig.hpp"
#include "main.hpp"
#include "al/byaml/writer/ByamlWriter.h"
#include "math/seadVector.h"
#include "rs/util/InputUtil.h"
#include "sead/prim/seadSafeString.h"
#include "server/freeze/FreezeTagMode.hpp"
#include "server/hns/HideAndSeekMode.hpp"
#include "game/Player/CapFunction.h"

bool checkpointPatch()
{
    if (GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG))
        return false;
    
    return true;
}

bool comboBtnHook(int port) {
    if(GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG))
        return false;

    if (GameModeManager::instance()->isActive()) { // only switch to combo if any gamemode is active
        return !al::isPadHoldL(port) && al::isPadTriggerDown(port);
    } else {
        return al::isPadTriggerDown(port);
    }
}

void saveWriteHook(al::ByamlWriter* saveByml) {

    const char *serverIP = Client::getCurrentIP();
    const int serverPort = Client::getCurrentPort();

    if (serverIP) {
        saveByml->addString("ServerIP", serverIP);
    } else {
        saveByml->addString("ServerIP", "127.0.0.1");
    }

    if (serverPort) {
        saveByml->addInt("ServerPort", serverPort);
    } else {
        saveByml->addInt("ServerPort", 0);
    }

    saveByml->pop();
}

bool saveReadHook(int* padRumbleInt, al::ByamlIter const& saveByml, char const* padRumbleKey) {

    const char *serverIP = "";
    int serverPort = 0;

    if (al::tryGetByamlString(&serverIP, saveByml, "ServerIP")) {
        Client::setLastUsedIP(serverIP);
    }

    if (al::tryGetByamlS32(&serverPort, saveByml, "ServerPort")) {
        Client::setLastUsedPort(serverPort);
    }
    
    return al::tryGetByamlS32(padRumbleInt, saveByml, padRumbleKey);
}

bool registerShineToList(Shine* shineActor) {

    if (shineActor->mShineIdx >= 0) {
        Client::tryRegisterShine(shineActor);
    } 

    return al::isAlive(shineActor);
}

// void overrideNerveHook(StageSceneStatePauseMenu* thisPtr, al::Nerve* nrvSet) {

//     if (al::isPadHoldZL(-1)) {
//         al::setNerve(thisPtr, &nrvStageSceneStatePauseMenuServerConfig);
//     } else {
//         al::setNerve(thisPtr, nrvSet);
//     }
// }

void overrideHelpFadeNerve(StageSceneStatePauseMenu* thisPtr)
{
    // Set label in menu inside LocalizedData/lang/MessageData/LayoutData/Menu.msbt
    thisPtr->exeServerConfig();
    al::setNerve(thisPtr, &nrvStageSceneStatePauseMenuServerConfig);
    return;
}

StageSceneStateServerConfig *sceneStateServerConfig = nullptr;

void initStateHook(StageSceneStatePauseMenu *thisPtr, char const *stateName, al::Scene *host, al::LayoutInitInfo const &initInfo, FooterParts *footer,
                   GameDataHolder *data, bool unkBool) {
    thisPtr->mStateOption =
        new StageSceneStateOption(stateName, host, initInfo, footer, data, unkBool);

    sceneStateServerConfig = new StageSceneStateServerConfig("ServerConfig", host, initInfo, footer, data, unkBool);
}

void initNerveStateHook(StageSceneStatePauseMenu* stateParent, StageSceneStateOption* stateOption,
                        al::Nerve const* executingNerve, char const* stateName) {

    al::initNerveState(stateParent, stateOption, executingNerve, stateName);

    al::initNerveState(stateParent, sceneStateServerConfig, &nrvStageSceneStatePauseMenuServerConfig, "CustomNerveOverride");
}

// skips starting both coin counters
void startCounterHook(CoinCounter* thisPtr) {
    if (!GameModeManager::instance()->isModeRequireUI()) {
        thisPtr->tryStart();
    }
}

// Simple hook that can be used to override isModeE3 checks to enable/disable certain behaviors
bool modeE3Hook() {
    return GameModeManager::instance()->isModeRequireUI();
}

// Skips ending the play guide layout if a mode is active, since the mode would have already ended it
void playGuideEndHook(al::SimpleLayoutAppearWaitEnd* thisPtr) {
    if (!GameModeManager::instance()->isModeRequireUI()) {
        thisPtr->end();
    }
}

// Gravity Hooks

void initHackCapHook(al::LiveActor *cappy) {
    al::initActorPoseTQGSV(cappy);
}

al::PlayerHolder* createTicketHook(StageScene* curScene) {
    // only creates custom gravity camera ticket if hide and seek mode is active
    if (GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK)) {
        al::CameraDirector* director = curScene->getCameraDirector();
        if (director) {
            if (director->mFactory) {
                al::CameraTicket* gravityCamera = director->createCameraFromFactory(
                    "CameraPoserCustom", nullptr, 0, 5, sead::Matrix34f::ident);

                HideAndSeekMode* mode = GameModeManager::instance()->getMode<HideAndSeekMode>();

                mode->setCameraTicket(gravityCamera);
            }
        }
    }

    if (GameModeManager::instance()->isMode(GameMode::FREEZETAG)) {
        al::CameraDirector* director = curScene->getCameraDirector();
        if (director) {
            if (director->mFactory) {
                al::CameraTicket* spectateCamera = director->createCameraFromFactory(
                    "CameraPoserActorSpectate", nullptr, 0, 5, sead::Matrix34f::ident);

                FreezeTagMode* mode = GameModeManager::instance()->getMode<FreezeTagMode>();

                mode->setCameraTicket(spectateCamera);
            }
        }
    }

    return al::getScenePlayerHolder(curScene);
}

bool borderPullBackHook(WorldEndBorderKeeper* thisPtr) {

    bool isFirstStep = al::isFirstStep(thisPtr);

    if (isFirstStep) {
        if (GameModeManager::instance()->isModeAndActive(GameMode::HIDEANDSEEK)) {

            HideAndSeekMode* mode = GameModeManager::instance()->getMode<HideAndSeekMode>();

            if (mode->isUseGravity()) {
                killMainPlayer(thisPtr->mActor);
            }
        }
    }
    
    return isFirstStep;
}

void drawTableHook(al::ExecuteDirector* thisPtr, const al::ExecuteSystemInitInfo &initInfo) {
    
    thisPtr->mUpdateTableCount = updateTableSize;
    thisPtr->mUpdateTables = new al::ExecuteTableHolderUpdate*[thisPtr->mUpdateTableCount]();

    for (int i = 0; i < thisPtr->mUpdateTableCount; i++) {
        thisPtr->mUpdateTables[i] = new al::ExecuteTableHolderUpdate();
        const al::ExecuteTable &curTable = updateTable[i];
        // Logger::log("Update Table Name: %s Count: %d\n", curTable.mName, curTable.mExecuteOrderCount);
        thisPtr->mUpdateTables[i]->init(curTable.mName, initInfo, curTable.mExecuteOrders, curTable.mExecuteOrderCount);
    }

    thisPtr->mDrawTableCount = drawTableSize;
    thisPtr->mDrawTables = new al::ExecuteTableHolderDraw*[thisPtr->mDrawTableCount]();
    
    for (int i = 0; i < thisPtr->mDrawTableCount; i++) {
        thisPtr->mDrawTables[i] = new al::ExecuteTableHolderDraw();
        const al::ExecuteTable* curTable = &drawTable[i];
        // Logger::log("Draw Table Name: %s Count: %d\n", curTable->mName, curTable->mExecuteOrderCount);
        thisPtr->mDrawTables[i]->init(curTable->mName, initInfo, curTable->mExecuteOrders, curTable->mExecuteOrderCount);
    }

    thisPtr->mRequestKeeper = new al::ExecuteRequestKeeper(thisPtr->mRequestMax);
}

void updateStateHook(al::Scene* scene) {
    al::executeUpdateList(scene->mActorKit, "OnlineUpdateExecutors", "PuppetActor");
    rs::updateEffectSystemEnv(scene);
}

void updateDrawHook(al::ExecuteDirector* thisPtr, const char* listName, const char* kit) {
    
    thisPtr->drawList("OnlineDrawExecutors", "PuppetActor");

    Logger::log("Updating Draw List for: %s %s\n", listName, kit);
    thisPtr->drawList(listName, kit);
}

float followDistHook() {
    switch(curSize){
        case NORMAL:
            return 330.f;
            break;
        case SMALL:
            return 330.f;
            break;
        case BIG:
            return 330.f;
            break;
            break;
        case VERYBIG:
            return 330.f;
            break;
    }
}
const char* offsetOverideHook(al::ByamlIter const& iter, char const* key) {
    switch(curSize){
        case SMALL:
             return "Y0.5m";
            break;
        default:
             return al::tryGetByamlKeyStringOrNULL(iter, key);
            break;
            break;
    }
}
PlayerConst* createPlayerConstHook(char const* suffix) {
    switch(curSize){
        case NORMAL: {
            PlayerConst* cons = PlayerFunction::createMarioConst(al::StringTmp<0x20>("Small%s", suffix).cstr());
            return cons;
            break;
        }
        case SMALL: {
            PlayerConst* cons = PlayerFunction::createMarioConst(al::StringTmp<0x20>("Small%s", suffix).cstr());
            return cons;
            break;
        }
        case BIG: {
            PlayerConst* cons = PlayerFunction::createMarioConst(al::StringTmp<0x20>("Small%s", suffix).cstr());
            return cons;
            break;
        }
        case VERYBIG: {
            PlayerConst* cons = PlayerFunction::createMarioConst(al::StringTmp<0x20>("Small%s", suffix).cstr());
            return cons;
            break;
        }
    }
}
// bool loadParamHook(float *output, al::ByamlIter const &iter, char const *key) {
//     bool result = al::tryGetByamlF32(output, iter, key);
//     if(isSmallMode)
//         *output = 0.2f;
//     return result;
// }

void effectHook(al::ActionEffectCtrl* effectController, char const* effectName) {
    if (curSize == PlayerSize::SMALL) {
        if(al::isEqualString(effectName, "RollingStart") || al::isEqualString(effectName, "Rolling") || al::isEqualString(effectName, "RollingStandUp") || al::isEqualString(effectName, "Jump") || al::isEqualString(effectName, "LandDownFall")|| al::isEqualString(effectName, "SpinCapStart") || al::isEqualString(effectName, "FlyingWaitR") || al::isEqualString(effectName, "StayR")|| al::isEqualString(effectName, "SpinGroundR")|| al::isEqualString(effectName, "StartSpinJumpR")|| al::isEqualString(effectName, "SpinJumpDownFallR")|| al::isEqualString(effectName, "Move")|| al::isEqualString(effectName, "Brake")) {
            al::tryDeleteEffect(effectController->mEffectKeeper, effectName);
            return;
        }
    }
    effectController->startAction(effectName);
}
void capVelScaleHook(al::LiveActor* hackCap, sead::Vector3f const& addition) {
    al::setVelocity(hackCap, addition * (scale * 0.8f));
}
void capReturnVelHook(HackCap *hackCap, sead::Vector3f const& addition) {
    if (curSize == PlayerSize::SMALL || curSize == PlayerSize::VERYBIG) {
        PlayerActorHakoniwa *pActor = (PlayerActorHakoniwa*)al::getPlayerActor(hackCap, 0);
        CapFunction::putOnCapPlayer(hackCap, pActor->mPlayerAnimator);
    } else {
        al::setVelocity(hackCap, addition);
    }
    
}
void spinFlowerHook(al::LiveActor* actor, float velocity) {
    al::addVelocityToGravity(actor, velocity * scale);
}
void sensorHook(al::LiveActor *actor, al::ActorInitInfo const &initInfo, char const *sensorName, uint typeEnum, float radius, ushort maxCount, sead::Vector3f const& position) {
    sead::Vector3f newPos = sead::Vector3f(position);
    if(position.y > 0)
        newPos.y = position.y * scale;
    al::addHitSensor(actor, initInfo, sensorName, typeEnum, radius, maxCount, newPos);
}
float fpHook() {
    return 300.0f * scale;
}
float fpScaleHook() {
    switch(curSize){
        case NORMAL:
            return 31.3f;
            break;
        case SMALL:
            return 31.3f;
            break;
        case BIG:
            return 31.3f;
            break;
            break;
        case VERYBIG:
            break;
    }
}
