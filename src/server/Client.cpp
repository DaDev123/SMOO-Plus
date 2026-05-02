#include "server/Client.hpp"

#include "hk/util/Math.h"

#include "nn/os.h"
#include "nn/socket.h"

#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorFlagFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/Play/Layout/SimpleLayoutAppearWaitEnd.h"

#include "game/MapObj/ChangeStageInfo.h"
#include "game/MapObj/CheckpointFlagWatcher.h"
#include "game/Player/HackCap.h"
#include "game/Player/PlayerAnimator.h"
#include "game/Player/PlayerAnimFrameCtrl.h"
#include "game/Player/PlayerHackKeeper.h"
#include "game/System/CustomGameDataFunction.h"
#include "game/System/GameDataFile.h"
#include "game/System/GameDataFunction.h"
#include "game/System/SaveDataAccessFunction.h"
#include "game/Util/ActorDimensionKeeper.h"

#include <cmath>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>

#include "heap/seadHeapMgr.h"
#include "helpers.hpp"
#include "Library/Base/StringUtil.h"
#include "Library/Layout/LayoutActorUtil.h"
#include "Library/LiveActor/LiveActor.h"
#include "logger.hpp"
#include "packets/Packet.h"
#include "server/SocketClient.hpp"
#include "System/GameDataHolder.h"
#include "System/GameDataHolderAccessor.h"
#include "System/GameDataHolderWriter.h"
#include "thread/seadMessageQueue.h"
#include "types.h"
#include "Util/AchievementUtil.h"

SEAD_SINGLETON_DISPOSER_IMPL(Client)

typedef void (Client::*ClientThreadFunc)(void);

/**
 * @brief Construct a new Client:: Client object
 *
 * @param bufferSize defines the maximum amount of puppets the client can handle
 */
Client::Client() {
    mHeap = sead::ExpHeap::create(500_KB, "ClientHeap", sead::HeapMgr::instance()->getCurrentHeap(), 8, sead::Heap::cHeapDirection_Forward, false);

    sead::ScopedCurrentHeapSetter heapSetter(mHeap);  // every new call after this will use ClientHeap instead of SequenceHeap

    mReadThread = new al::AsyncFunctorThread("ClientReadThread", al::FunctorV0M<Client*, ClientThreadFunc>(this, &Client::readFunc), 0, 0x1000, {0});

    mKeyboard = new Keyboard(nn::swkbd::GetRequiredStringBufferSize());

    mSocket = new SocketClient("SocketClient", mHeap);

    mPuppetHolder = new PuppetHolder(maxPuppets);

    for (size_t i = 0; i < MAXPUPINDEX; i++) {
        mPuppetInfoArr[i] = new PuppetInfo();

        sprintf(mPuppetInfoArr[i]->puppetName, "Puppet%zu", i);
    }

    strcpy(mDebugPuppetInfo.puppetName, "PuppetDebug");

    mMessageQueue.allocate(sMaxMsgCount, mHeap);

    mConnectCount = 0;

    curCollectedShines.fill(-1);

    collectedShineCount = 0;

    mShineArray.allocBuffer(100, nullptr);  // max of 100 shine actors in buffer

    mCoinCollectArray.allocBuffer(100, nullptr);

    mCoinCollect2DArray.allocBuffer(25, nullptr);

    mPendingCoinCollectCount = 0;

    mPendingCheckpointCount = 0;

    nn::account::GetLastOpenedUser(&mUserID);

    nn::account::Nickname playerName;
    nn::account::GetNickname(&playerName, mUserID);
    Logger::setLogName(playerName.name);  // set Debug logger name to player name

    mUsername = playerName.name;

    mUserID.print();

    Logger::log("Player Name: %s\n", playerName.name);

    Logger::log("%s Build Number: %s\n", playerName.name, TOSTRING(BUILDVERSTR));
}

/**
 * @brief initializes client class using initInfo obtained from StageScene::init
 *
 * @param initInfo init info used to create layouts used by client
 */
void Client::init(al::LayoutInitInfo const& initInfo, GameDataHolderAccessor holder) {
    mUIMessage = new (mHeap) al::WindowConfirmWait("ServerWaitConnect", "WindowConfirmWait", initInfo);

    mConnectStatus = new (mHeap) al::SimpleLayoutAppearWaitEnd("", "SaveMessage", initInfo, 0, false);

    mUIMessage->setTxtMessage(u"Connecting to Server.");
    mUIMessage->setTxtMessageConfirm(u"Failed to Connect!");

    al::setPaneString(mConnectStatus, "TxtSave", u"Connecting to Server.", 0);
    al::setPaneString(mConnectStatus, "TxtSaveSh", u"Connecting to Server.", 0);

    mHolder = holder;

    startThread();

    Logger::log("Heap Free Size: %f/%f\n", mHeap->getFreeSize() * 0.001f, mHeap->getSize() * 0.001f);
}

Client* Client::get() {
    return sInstance;
}

/**
 * @brief starts client read thread
 *
 * @return true if read thread was sucessfully started
 * @return false if read thread was unable to start, or thread was already started.
 */
bool Client::startThread() {
    if (mReadThread->isDone()) {
        mReadThread->start();
        Logger::log("Read Thread Sucessfully Started.\n");
        return true;
    } else {
        Logger::log("Read Thread has already started! Or other unknown reason.\n");
        return false;
    }
}

void Client::restartConnection() {
    if (!sInstance->mIsAllowReconnect)
        return;

    // send disconnect packet
    Packet* dc = new (sInstance->mHeap) Packet();
    dc->mType = PacketType::PLAYERDC;
    dc->mUserID = Client::getClientId();
    sInstance->mSocket->send(dc);
    sInstance->mHeap->free(dc);

    // close socket
    if (sInstance->mSocket->closeSocket()) {
        Logger::log("Successfully Closed Socket.\n");
    }

    sInstance->mConnectCount = 0;
    for (PuppetInfo* curInfo : sInstance->mPuppetInfoArr) {
        curInfo->isConnected = false;

        curInfo->scenarioNo = -1;
        strcpy(curInfo->stageName, "");
        curInfo->isInSameStage = false;
    }

    sInstance->mSocket->setLogState(SOCKET_LOG_DISCONNECTED);
    sInstance->mSocket->startEndThread();

    sInstance->mIsConnectionActive = sInstance->mSocket->init(sInstance->mServerIP.cstr(), sInstance->mServerPort).IsSuccess();

    nn::os::SleepThread(nn::TimeSpan::FromMilliSeconds(10));  // BAD

    if (sInstance->lastGameInfPacket != sInstance->emptyGameInfPacket) {
        if (sInstance->lastGameInfPacket.mUserID != sInstance->mUserID) {
            sInstance->lastGameInfPacket.mUserID = sInstance->mUserID;
        }
        sInstance->mSocket->send(&sInstance->lastGameInfPacket);
    }

    if (sInstance->lastPlayerInfPacket.mUserID == sInstance->mUserID) {
        sInstance->mSocket->send(&sInstance->lastPlayerInfPacket);
    }

    if (sInstance->lastCostumeInfPacket.bodyModel[0] != '\0') {
        sInstance->lastCostumeInfPacket.mUserID = sInstance->mUserID;
        sInstance->mSocket->send(&sInstance->lastCostumeInfPacket);
    }

    sInstance->lastCaptureInfPacket.mUserID = sInstance->mUserID;
    sInstance->mSocket->send(&sInstance->lastCaptureInfPacket);
}

/**
 * @brief starts a connection using client's TCP socket class, pulling up the software keyboard for
 * user inputted IP if save file does not have one saved.
 *
 * @return true if successful connection to server
 * @return false if connection was unable to establish
 */
bool Client::startConnection() {
    bool isNeedSave = false;

    bool isOverride = al::isPadHoldZL(-1);

    if (mServerIP.isEmpty() || isOverride) {
        mKeyboard->setHeaderText(u"Save File does not contain an IP!");
        mKeyboard->setSubText(u"Please set a Server IP Below.");
        mServerIP = "127.0.0.1";
        Client::openKeyboardIP();
        isNeedSave = true;
    }

    if (!mServerPort || isOverride) {
        mKeyboard->setHeaderText(u"Save File does not contain a port!");
        mKeyboard->setSubText(u"Please set a Server Port Below.");
        mServerPort = 1027;
        Client::openKeyboardPort();
        isNeedSave = true;
    }

    if (isNeedSave) {
        SaveDataAccessFunction::startSaveDataWrite(mHolder.mData);
    }

    mIsConnectionActive = mSocket->init(mServerIP.cstr(), mServerPort).IsSuccess();

    if (mIsConnectionActive) {
        Logger::log("Sucessful Connection. Waiting to recieve init packet.\n");

        bool waitingForInitPacket = true;

        while (waitingForInitPacket == true) {
            Packet* curPacket = mSocket->tryGetPacket();

            if (curPacket) {
                if (curPacket->mType == PacketType::CLIENTINIT) {
                    InitPacket* initPacket = (InitPacket*)curPacket;

                    Logger::log("Server Max Player Size: %d\n", initPacket->maxPlayers);

                    maxPuppets = initPacket->maxPlayers - 1;
                    mPuppetHolder->resizeHolder(maxPuppets);

                    if (al::isStartWithString(initPacket->ServerVersion, "SMOO+")) {
                        sInstance->mIsAllowReconnect = true;
                    } else {
                        sInstance->mIsAllowReconnect = false;
                    }

                    setServerVersion(initPacket->ServerVersion);
                    Logger::log("Server version: %s\n", initPacket->ServerVersion);

                    waitingForInitPacket = false;
                }

                free(curPacket);
            } else {
                Logger::log("Recieve failed! Stopping Connection.\n");
                mIsConnectionActive = false;
                waitingForInitPacket = false;
            }
        }
    }

    return mIsConnectionActive;
}

/**
 * @brief Opens up OS's software keyboard in order to change the currently used server IP.
 * @returns whether or not a new IP has been defined and needs to be saved.
 */
bool Client::openKeyboardIP() {
    if (!sInstance) {
        Logger::log("Static Instance is null!\n");
        return false;
    }

    sInstance->mKeyboard->openKeyboard(sInstance->mServerIP.cstr(), [](nn::swkbd::KeyboardConfig& config) {
        config.keyboardMode = nn::swkbd::KeyboardMode::ModeASCII;
        config.textMaxLength = MAX_HOSTNAME_LENGTH;
        config.textMinLength = 1;
        config.isUseUtf8 = true;
        config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
    });

    hostname prevIp = sInstance->mServerIP;

    while (true) {
        if (sInstance->mKeyboard->isThreadDone()) {
            if (!sInstance->mKeyboard->isKeyboardCancelled())
                sInstance->mServerIP = sInstance->mKeyboard->getResult();
            break;
        }
        nn::os::YieldThread();
    }

    bool isFirstConnect = prevIp != sInstance->mServerIP;

    sInstance->mSocket->setIsFirstConn(isFirstConnect);

    return isFirstConnect;
}

/**
 * @brief Opens up OS's software keyboard in order to change the currently used server port.
 * @returns whether or not a new port has been defined and needs to be saved.
 */
bool Client::openKeyboardPort() {
    if (!sInstance) {
        Logger::log("Static Instance is null!\n");
        return false;
    }

    char buf[6];
    nn::util::SNPrintf(buf, 6, "%u", sInstance->mServerPort);

    sInstance->mKeyboard->openKeyboard(buf, [](nn::swkbd::KeyboardConfig& config) {
        config.keyboardMode = nn::swkbd::KeyboardMode::ModeNumeric;
        config.textMaxLength = 5;
        config.textMinLength = 2;
        config.isUseUtf8 = true;
        config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
    });

    int prevPort = sInstance->mServerPort;

    while (true) {
        if (sInstance->mKeyboard->isThreadDone()) {
            if (!sInstance->mKeyboard->isKeyboardCancelled())
                sInstance->mServerPort = ::atoi(sInstance->mKeyboard->getResult());
            break;
        }
        nn::os::YieldThread();
    }

    bool isFirstConnect = prevPort != sInstance->mServerPort;

    sInstance->mSocket->setIsFirstConn(isFirstConnect);

    return isFirstConnect;
}

/**
 * @brief Sets the server IP address
 * @param ip The IP address or hostname to set
 */
void Client::setServerIP(const char* ip) {
    if (!sInstance) {
        Logger::log("Static Instance is null!\n");
        return;
    }

    hostname prevIp = sInstance->mServerIP;
    sInstance->mServerIP = ip;

    bool isFirstConnect = prevIp != sInstance->mServerIP;
    sInstance->mSocket->setIsFirstConn(isFirstConnect);
}

/**
 * @brief Sets the server port
 * @param port The port number to set
 */
void Client::setServerPort(int port) {
    if (!sInstance) {
        Logger::log("Static Instance is null!\n");
        return;
    }

    int prevPort = sInstance->mServerPort;
    sInstance->mServerPort = port;

    bool isFirstConnect = prevPort != sInstance->mServerPort;
    sInstance->mSocket->setIsFirstConn(isFirstConnect);
}

void Client::showUIMessage(const char16_t* msg) {
    if (!sInstance) {
        return;
    }

    sInstance->mUIMessage->setTxtMessageConfirm(msg);

    al::hidePane(sInstance->mUIMessage, "Page01");  // hide A button prompt

    if (!sInstance->mUIMessage->mIsAlive) {
        sInstance->mUIMessage->appear();

        sInstance->mUIMessage->playLoop();
    }

    al::startAction(sInstance->mUIMessage, "Confirm", "State");
}

void Client::hideUIMessage() {
    if (!sInstance) {
        return;
    }

    sInstance->mUIMessage->tryEnd();
}

/**
 * @brief main thread function for read thread, responsible for processing packets from server
 *
 */
void Client::readFunc() {
    Logger::log("Starting Client read thread\n");

    if (waitForGameInit) {
        nn::os::YieldThread();
        nn::os::SleepThread(nn::TimeSpan::FromSeconds(2));
        waitForGameInit = false;
    }

    mConnectStatus->appear();

    al::startAction(mConnectStatus, "Loop", "Loop");

    if (!startConnection()) {
        Logger::log("Failed to Connect to Server.\n");

        nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(250000000));

        mConnectStatus->end();

        return;
    }

    nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(500000000));

    mConnectStatus->end();

    while (mIsConnectionActive) {
        Packet* curPacket = mSocket->tryGetPacket();

        if (curPacket) {
            switch (curPacket->mType) {
            case PacketType::PLAYERINF:
                updatePlayerInfo((PlayerInf*)curPacket);
                break;
            case PacketType::GAMEINF:
                updateGameInfo((GameInf*)curPacket);
                break;
            case PacketType::HACKCAPINF:
                updateHackCapInfo((HackCapInf*)curPacket);
                break;
            case PacketType::CAPTUREINF:
                updateCaptureInfo((CaptureInf*)curPacket);
                break;
            case PacketType::PLAYERCON:
                updatePlayerConnect((PlayerConnect*)curPacket);

                if (lastGameInfPacket != emptyGameInfPacket) {
                    if (lastGameInfPacket.mUserID != mUserID) {
                        lastGameInfPacket.mUserID = mUserID;
                    }
                    mSocket->send(&lastGameInfPacket);
                }

                if (lastPlayerInfPacket.mUserID == mUserID) {
                    mSocket->send(&lastPlayerInfPacket);
                }
                if (lastCostumeInfPacket.bodyModel[0] != '\0') {
                    lastCostumeInfPacket.mUserID = mUserID;
                    mSocket->send(&lastCostumeInfPacket);
                }

                lastCaptureInfPacket.mUserID = mUserID;
                mSocket->send(&lastCaptureInfPacket);

                break;
            case PacketType::COSTUMEINF:
                updateCostumeInfo((CostumeInf*)curPacket);
                break;
            case PacketType::SHINECOLL:
                updateShineInfo((ShineCollect*)curPacket);
                break;
            case PacketType::MESSAGE:
                updateMessages((MessagePacket*)curPacket);
                break;
            case PacketType::PLAYERDC:
                Logger::log("Received Player Disconnect!\n");
                curPacket->mUserID.print();
                disconnectPlayer((PlayerDC*)curPacket);
                break;
            case PacketType::TAGINF:
                updateTagInfo((TagInf*)curPacket);
                break;
            case PacketType::CHANGESTAGE:
                sendToStage((ChangeStagePacket*)curPacket);
                break;
            case PacketType::HEALTHCOINS:
                updateHealthCoins((HealthCoins*)curPacket);
                break;

            case PacketType::CLIENTINIT: {
                InitPacket* initPacket = (InitPacket*)curPacket;
                Logger::log("Server Max Player Size: %d\n", initPacket->maxPlayers);
                maxPuppets = initPacket->maxPlayers - 1;
                mPuppetHolder->resizeHolder(maxPuppets);
                if (al::isStartWithString(initPacket->ServerVersion, "SMOO+")) {
                    sInstance->mIsAllowReconnect = true;
                } else {
                    sInstance->mIsAllowReconnect = false;
                }
                setServerVersion(initPacket->ServerVersion);
                Logger::log("Server version: ", initPacket->ServerVersion);
                break;
            }
            default:
                Logger::log("Discarding Unknown Packet Type.\n");
                break;
            }

            free(curPacket);

        } else {
            Logger::log("Client Socket Encountered an Error! Errno: 0x%x\n", mSocket->socket_errno);
        }
    }

    Logger::log("Client Read Thread ending.\n");
}

void Client::sendPlayerInfPacket(const PlayerActorBase* playerBase, bool isYukimaru) {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    if (!playerBase) {
        Logger::log("Error: Null Player Reference\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    PlayerInf* packet = new PlayerInf();
    packet->mUserID = sInstance->mUserID;

    packet->playerPos = al::getTrans(playerBase);

    al::calcQuat(&packet->playerRot, playerBase);

    if (!isYukimaru) {
        PlayerActorHakoniwa* player = (PlayerActorHakoniwa*)playerBase;

        for (size_t i = 0; i < 6; i++) {
            packet->animBlendWeights[i] = player->mAnimator->getBlendWeight(i);
        }

        const char* hackName = player->mHackKeeper->getCurrentHackName();

        if (hackName != nullptr) {
            sInstance->isClientCaptured = true;

            const char* actName = al::getActionName(player->mHackKeeper->mHackActor);

            if (actName) {
                packet->actName = PlayerAnims::FindType(actName);
                packet->subActName = PlayerAnims::Type::Unknown;
            } else {
                packet->actName = PlayerAnims::Type::Unknown;
                packet->subActName = PlayerAnims::Type::Unknown;
            }
        } else {
            packet->actName = PlayerAnims::FindType(player->mAnimator->mAnimFrameCtrl->getActionName());
            packet->subActName = PlayerAnims::FindType(player->mAnimator->mCurSubAnim.cstr());

            sInstance->isClientCaptured = false;
        }

    } else {
        // TODO: implement YukimaruRacePlayer syncing

        for (size_t i = 0; i < 6; i++) {
            packet->animBlendWeights[i] = 0;
        }

        sInstance->isClientCaptured = false;

        packet->actName = PlayerAnims::Type::Unknown;
        packet->subActName = PlayerAnims::Type::Unknown;
    }

    if (sInstance->lastPlayerInfPacket != *packet) {
        sInstance->lastPlayerInfPacket = *packet;
        sInstance->mSocket->queuePacket(packet);
    } else {
        sInstance->mHeap->free(packet);
    }
}

/**
 * @brief sends info related to player's cap actor to server
 *
 * @param hackCap pointer to cap actor, used to get translation, animation, and state info
 */
void Client::sendHackCapInfPacket(const HackCap* hackCap) {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    bool isFlying = hackCap->isFlying();

    if (isFlying) {
        HackCapInf* packet = new HackCapInf();
        packet->mUserID = sInstance->mUserID;
        packet->capPos = al::getTrans(hackCap);

        packet->isCapVisible = isFlying;

        packet->capQuat.x = hackCap->mJointKeeper->mJointRot.x;
        packet->capQuat.y = hackCap->mJointKeeper->mJointRot.y;
        packet->capQuat.z = hackCap->mJointKeeper->mJointRot.z;
        packet->capQuat.w = hackCap->mJointKeeper->mSkew;
        packet->capRotQuat = al::getQuat(hackCap);

        strcpy(packet->capAnim, al::getActionName(hackCap));

        sInstance->mSocket->queuePacket(packet);

        sInstance->isSentHackInf = true;

    } else if (sInstance->isSentHackInf) {
        HackCapInf* packet = new HackCapInf();
        packet->mUserID = sInstance->mUserID;
        packet->isCapVisible = false;
        packet->capPos = sead::Vector3f::zero;
        packet->capQuat = sead::Quatf::unit;
        packet->capRotQuat = sead::Quatf::unit;
        sInstance->mSocket->queuePacket(packet);
        sInstance->isSentHackInf = false;
    }
}

/**
 * @brief Sends both stage info and player 2D info to the server.
 * @param player
 * @param holder
 */
void Client::sendGameInfPacket(const PlayerActorHakoniwa* player, GameDataHolderAccessor holder) {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    GameInf* packet = new GameInf();
    packet->mUserID = sInstance->mUserID;

    if (player) {
        packet->is2D = player->mDimensionKeeper->mIs2D;
    } else {
        packet->is2D = false;
    }

    packet->scenarioNo = holder.mData->getGameDataFile()->getScenarioNo();

    strcpy(packet->stageName, GameDataFunction::getCurrentStageName(holder));

    packet->gameMode = -1;

    if (*packet != sInstance->lastGameInfPacket) {
        sInstance->lastGameInfPacket = *packet;
        sInstance->mSocket->queuePacket(packet);
    } else {
        sInstance->mHeap->free(packet);
    }
}

/**
 * @brief Sends only stage info to the server.
 * @param holder
 */
void Client::sendGameInfPacket(GameDataHolderAccessor holder) {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    GameInf* packet = new GameInf();
    packet->mUserID = sInstance->mUserID;

    packet->is2D = false;

    packet->scenarioNo = holder.mData->getGameDataFile()->getScenarioNo();

    strcpy(packet->stageName, GameDataFunction::getCurrentStageName(holder));

    packet->gameMode = -1;

    sInstance->lastGameInfPacket = *packet;

    sInstance->mSocket->queuePacket(packet);
}

/**
 * @brief Sends tag info packet for H&S or Sardine modes.
 */
void Client::sendTagInfPacket() {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    TagInf* packet = new TagInf();

    packet->mUserID = sInstance->mUserID;

    packet->updateType = static_cast<TagUpdateType>(TagUpdateType::STATE | TagUpdateType::TIME);

    sInstance->mSocket->queuePacket(packet);
}

void Client::sendFreezeInfPacket() {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);
}

void Client::sendShineThiefInfPacket() {
    if (!sInstance) {
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);
}

/**
 * @brief Sends costume info packet.
 * @param body
 * @param cap
 */
void Client::sendCostumeInfPacket(const char* body, const char* cap) {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    CostumeInf* packet = new CostumeInf(body, cap);
    packet->mUserID = sInstance->mUserID;
    sInstance->lastCostumeInfPacket = *packet;
    sInstance->mSocket->queuePacket(packet);
}

/**
 * @brief Sends capture info packet.
 * @param player
 */
void Client::sendCaptureInfPacket(const PlayerActorHakoniwa* player) {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    if (sInstance->isClientCaptured && !sInstance->isSentCaptureInf) {
        CaptureInf* packet = new CaptureInf();
        packet->mUserID = sInstance->mUserID;
        strcpy(packet->hackName, tryConvertName(player->mHackKeeper->getCurrentHackName()));
        sInstance->mSocket->queuePacket(packet);
        sInstance->isSentCaptureInf = true;
    } else if (!sInstance->isClientCaptured && sInstance->isSentCaptureInf) {
        CaptureInf* packet = new CaptureInf();
        packet->mUserID = sInstance->mUserID;
        strcpy(packet->hackName, "");
        sInstance->mSocket->queuePacket(packet);
        sInstance->isSentCaptureInf = false;
    }
}

/**
 * @brief Sends shine collect packet.
 * @param shineID
 */
void Client::sendShineCollectPacket(int shineID) {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    if (sInstance->lastCollectedShine != shineID) {
        ShineCollect* packet = new ShineCollect();
        packet->mUserID = sInstance->mUserID;
        packet->shineId = shineID;

        sInstance->lastCollectedShine = shineID;

        sInstance->mSocket->queuePacket(packet);
    }
}

/**
 * @brief Updates player info from packet.
 * @param packet
 */
void Client::updatePlayerInfo(PlayerInf* packet) {
    PuppetInfo* curInfo = findPuppetInfo(packet->mUserID, false);

    if (!curInfo) {
        return;
    }

    if (!curInfo->isConnected) {
        curInfo->isConnected = true;
    }

    curInfo->playerPos = packet->playerPos;

    if (abs(packet->playerRot.x) > 0.f || abs(packet->playerRot.y) > 0.f || abs(packet->playerRot.z) > 0.f || abs(packet->playerRot.w) > 0.f) {
        if (abs(packet->playerRot.x) <= 1.f || abs(packet->playerRot.y) <= 1.f || abs(packet->playerRot.z) <= 1.f || abs(packet->playerRot.w) <= 1.f) {
            curInfo->playerRot = packet->playerRot;
        }
    }

    if (packet->actName != PlayerAnims::Type::Unknown) {
        strcpy(curInfo->curAnimStr, PlayerAnims::FindStr(packet->actName));
        if (curInfo->curAnimStr[0] == '\0')
            Logger::log("[ERROR] %s: actName was out of bounds: %d\n", __func__, packet->actName);
    } else {
        strcpy(curInfo->curAnimStr, "Wait");
    }

    if (packet->subActName != PlayerAnims::Type::Unknown) {
        strcpy(curInfo->curSubAnimStr, PlayerAnims::FindStr(packet->subActName));
        if (curInfo->curSubAnimStr[0] == '\0')
            Logger::log("[ERROR] %s: subActName was out of bounds: %d\n", __func__, packet->subActName);
    } else {
        strcpy(curInfo->curSubAnimStr, "");
    }

    curInfo->curAnim = packet->actName;
    curInfo->curSubAnim = packet->subActName;

    for (size_t i = 0; i < 6; i++) {
        if (packet->animBlendWeights[i] >= 0.f && packet->animBlendWeights[i] <= 1.f) {
            curInfo->blendWeights[i] = packet->animBlendWeights[i];
        }
    }

    if (!curInfo->isCapThrow) {
        curInfo->capPos = packet->playerPos;
    }
}

/**
 * @brief Updates hack cap info from packet.
 * @param packet
 */
void Client::updateHackCapInfo(HackCapInf* packet) {
    PuppetInfo* curInfo = findPuppetInfo(packet->mUserID, false);
    if (!curInfo)
        return;
    bool isOldPacket = packet->mPacketSize == (sizeof(HackCapInf) - sizeof(Packet) - sizeof(sead::Quatf));

    curInfo->capPos = packet->capPos;

    if (isOldPacket) {
        struct PACKED OldHackCapInf {
            sead::Vector3f capPos;
            sead::Quatf capQuat;
            bool1 isCapVisible;
            char capAnim[PACKBUFSIZE];
        };
        auto* old = reinterpret_cast<OldHackCapInf*>(&packet->capPos);
        curInfo->capRot = old->capQuat;
        curInfo->capQuat = {0.f, 0.f, 0.f, 0.f};
        curInfo->isCapThrow = old->isCapVisible;
        strcpy(curInfo->capAnim, old->capAnim);
    } else {
        curInfo->capRot = packet->capQuat;
        curInfo->capQuat = packet->capRotQuat;
        curInfo->isCapThrow = packet->isCapVisible;
        strcpy(curInfo->capAnim, packet->capAnim);
    }
}

/**
 * @brief Updates capture info from packet.
 * @param packet
 */
void Client::updateCaptureInfo(CaptureInf* packet) {
    PuppetInfo* curInfo = findPuppetInfo(packet->mUserID, false);

    if (!curInfo) {
        return;
    }

    curInfo->isCaptured = strlen(packet->hackName) > 0;

    if (curInfo->isCaptured) {
        strcpy(curInfo->curHack, packet->hackName);
    }
}

/**
 * @brief Updates costume info from packet.
 * @param packet
 */
void Client::updateCostumeInfo(CostumeInf* packet) {
    PuppetInfo* curInfo = findPuppetInfo(packet->mUserID, false);

    if (!curInfo) {
        return;
    }

    strcpy(curInfo->costumeBody, packet->bodyModel);
    strcpy(curInfo->costumeHead, packet->capModel);
}

/**
 * @brief Stores shine ID from packet for later processing on the game thread.
 * @param packet
 */
void Client::updateShineInfo(ShineCollect* packet) {
    if (collectedShineCount < curCollectedShines.size() - 1) {
        curCollectedShines[collectedShineCount] = packet->shineId;
        collectedShineCount++;
    }
}

/**
 * @brief Updates player connect info from packet.
 * @param packet
 */
void Client::updatePlayerConnect(PlayerConnect* packet) {
    PuppetInfo* curInfo = findPuppetInfo(packet->mUserID, true);

    if (!curInfo) {
        return;
    }

    if (curInfo->isConnected) {
        Logger::log("Info is already being used by another connected player!\n");
        packet->mUserID.print("Connection ID");
        curInfo->playerID.print("Target Info");

    } else {
        packet->mUserID.print("Player Connected! ID");

        curInfo->playerID = packet->mUserID;
        curInfo->isConnected = true;
        strcpy(curInfo->puppetName, packet->clientName);

        mConnectCount++;
    }
}

const struct {
    const char* stage;
    s32 index;
    const char* warpStage;
    int possibleScenarios[5];
    int noSyncScen = -1;

} stageListForScenarioSync[] = {
    {"CapWorldHomeStage", 0, "CapWorldHomeStage", {1, 2, 3, 4, 0}, 1},
    {"WaterfallWorldHomeStage", 1, "WaterfallWorldHomeStage", {1, 2, 3, 4, 0}},
    {"SandWorldHomeStage", 2, "SandWorldHomeStage", {1, 2, 3, 4, 5}},
    {"SandWorldUnderground001Stage", 2, "SandWorldHomeStage", {1, 2, 3, 4, 5}},
    {"ForestWorldHomeStage", 3, "ForestWorldHomeStage", {1, 2, 3, 4, 5}},
    {"ForestWorldBossStage", 3, "ForestWorldHomeStage", {1, 2, 3, 4, 5}},
    {"LakeWorldHomeStage", 4, "LakeWorldHomeStage", {1, 2, 3, 4, 0}},
    {"CloudWorldHomeStage", 5, "CloudWorldHomeStage", {1, 3, 4, 0, 0}, 1},
    {"ClashWorldHomeStage", 6, "ClashWorldHomeStage", {1, 2, 3, 4, 0}},
    {"CityWorldHomeStage", 7, "CityWorldHomeStage", {1, 2, 4, 5, 8}},
    {"SeaWorldHomeStage", 8, "SeaWorldHomeStage", {1, 2, 3, 4, 0}},
    {"SnowWorldHomeStage", 9, "SnowWorldHomeStage", {1, 2, 3, 4, 0}},
    {"SnowWorldLobby001Stage", 9, "SnowWorldHomeStage", {1, 2, 3, 4, 0}},
    {"LavaWorldHomeStage", 10, "LavaWorldHomeStage", {1, 2, 3, 4, 8}},
    {"BossRaidWorldHomeStage", 11, "BossRaidWorldHomeStage", {1, 2, 3, 4, 0}},
    {"SkyWorldHomeStage", 12, "SkyWorldHomeStage", {1, 2, 3, 4, 0}},
    {"MoonWorldHomeStage", 13, "MoonWorldHomeStage", {1, 2, 3, 0, 0}},
    {"PeachWorldHomeStage", 14, "PeachWorldHomeStage", {2, 0, 0, 0, 0}},
    {"Special1WorldHomeStage", 15, "Special1WorldHomeStage", {1, 2, 0, 0, 0}},
    {"Special2WorldHomeStage", 16, "Special2WorldHomeStage", {1, 2, 0, 0, 0}},
};

static s32 findWorldIdFromStageName(const char* stageName) {
    for (s32 i = 0; i < hk::util::arraySize(stageListForScenarioSync); i++) {
        if (al::isEqualString(stageListForScenarioSync[i].stage, stageName)) {
            return stageListForScenarioSync[i].index;
        }
    }
    return -1;
}

static bool validateScenarioFromStageName(const char* stageName, s32 scenario, s32 curscen) {
    for (s32 i = 0; i < hk::util::arraySize(stageListForScenarioSync); i++) {
        if (al::isEqualString(stageListForScenarioSync[i].stage, stageName)) {
            if (curscen == stageListForScenarioSync[i].noSyncScen)
                return false;

            for (s32 j = 0; j < hk::util::arraySize(stageListForScenarioSync[i].possibleScenarios); j++) {
                if (stageListForScenarioSync[i].possibleScenarios[j] == scenario)
                    return true;
            }
        }
    }
    return false;
}

static const char* findWarpStageFromStageName(const char* stageName) {
    for (s32 i = 0; i < hk::util::arraySize(stageListForScenarioSync); i++) {
        if (al::isEqualString(stageListForScenarioSync[i].stage, stageName)) {
            return stageListForScenarioSync[i].warpStage;
        }
    }
    return nullptr;
}

/**
 * @brief Updates game info from packet, and handles scenario sync.
 * @param packet
 */
void Client::updateGameInfo(GameInf* packet) {
    PuppetInfo* curInfo = findPuppetInfo(packet->mUserID, false);

    if (!curInfo) {
        return;
    }

    if (curInfo->isConnected) {
        curInfo->scenarioNo = packet->scenarioNo;

        if (strcmp(packet->stageName, "") != 0 && strlen(packet->stageName) > 3) {
            strcpy(curInfo->stageName, packet->stageName);
        }

        curInfo->is2D = packet->is2D;
        curInfo->gameMode = packet->gameMode;
    }

    if (findWorldIdFromStageName(packet->stageName) == -1)
        return;
    GameDataFile::FixedHeapArray<s32, sNumWorlds> scenNumArr = Client::sInstance->getHolder()->getGameDataFile()->getScenarioNumArr();
    GameDataFile::FixedHeapArray<s32, sNumWorlds> mainSenNumArr = Client::sInstance->getHolder()->getGameDataFile()->getMainScenarioNumArr();

    int curScen = scenNumArr[findWorldIdFromStageName(packet->stageName)];
    if (packet->scenarioNo < 15 && packet->scenarioNo > curScen && validateScenarioFromStageName(packet->stageName, packet->scenarioNo, curScen)) {
        scenNumArr[findWorldIdFromStageName(packet->stageName)] = packet->scenarioNo;
        mainSenNumArr[findWorldIdFromStageName(packet->stageName)] = packet->scenarioNo;
        const char* warpStage = findWarpStageFromStageName(packet->stageName);
        if (warpStage && strcmp(GameDataFunction::getCurrentStageName(Client::getHolder()), warpStage) == 0) {
            ChangeStageInfo info(Client::getHolder(), "start", warpStage, false, packet->scenarioNo);
            Client::getHolder()->changeNextStage(&info);
        }
    }
}

/**
 * @brief Queues a message packet for the game thread to consume.
 * @param packet
 */
void Client::updateMessages(MessagePacket* packet) {
    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);
    MessagePacket* p = new MessagePacket();

    *p = *packet;

    if (mMessageQueue.mMessageQueueInner._count != sMaxMsgCount)
        mMessageQueue.push((s64)p, sead::MessageQueue::BlockType::NonBlocking);
    else
        sInstance->mHeap->free(p);
}

/**
 * @brief Updates tag info from packet for H&S, Sardine, Freeze Tag, and Shine Thief modes.
 * @param packet
 */
void Client::updateTagInfo(TagInf* packet) {}

/**
 * @brief Sends the player to a new stage as directed by a ChangeStagePacket.
 * @param packet
 */
void Client::sendToStage(ChangeStagePacket* packet) {
    if (mSceneInfo && mSceneInfo->sceneObjHolder) {
        GameDataHolderWriter accessor(mSceneInfo->sceneObjHolder);

        Logger::log("Sending Player to %s at Entrance %s in Scenario %d\n", packet->changeStage, packet->changeID, packet->scenarioNo);

        ChangeStageInfo info(accessor.mData, packet->changeID, packet->changeStage, false, packet->scenarioNo,
                             static_cast<ChangeStageInfo::SubScenarioType>(packet->subScenarioType));
        info.setWipeType("FadeBlack");
        GameDataFunction::tryChangeNextStage(accessor, &info);
    }
}

/**
 * @brief Handles a player disconnect packet.
 * @param packet
 */
void Client::disconnectPlayer(PlayerDC* packet) {
    PuppetInfo* curInfo = findPuppetInfo(packet->mUserID, false);

    if (!curInfo || !curInfo->isConnected) {
        return;
    }

    curInfo->isConnected = false;

    curInfo->scenarioNo = -1;
    strcpy(curInfo->stageName, "");
    curInfo->isInSameStage = false;

    mConnectCount--;
    mShouldStopRumble = true;
}

/**
 * @brief Checks if a shine has already been collected this session.
 * @param shineId
 */
bool Client::isShineCollected(int shineId) {
    for (size_t i = 0; i < curCollectedShines.size(); i++) {
        if (curCollectedShines[i] >= 0) {
            if (curCollectedShines[i] == shineId) {
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief Finds a PuppetInfo slot by user ID.
 * @param id
 * @param isFindAvailable if true, returns first free slot when no match found
 */
PuppetInfo* Client::findPuppetInfo(const nn::account::Uid& id, bool isFindAvailable) {
    PuppetInfo* firstAvailable = nullptr;

    for (size_t i = 0; i < getMaxPlayerCount() - 1; i++) {
        PuppetInfo* curInfo = mPuppetInfoArr[i];

        if (curInfo->playerID == id) {
            return curInfo;
        } else if (isFindAvailable && !firstAvailable && !curInfo->isConnected) {
            firstAvailable = curInfo;
        }
    }

    if (!firstAvailable) {
        Logger::log("Unable to find Assigned Puppet for Player!\n");
        id.print("User ID");
    }

    return firstAvailable;
}

/**
 * @brief Sets the current stage name and scenario number, and updates the puppet holder.
 * @param holder
 */
void Client::setStageInfo(GameDataHolderAccessor holder) {
    if (sInstance) {
        sInstance->mStageName = GameDataFunction::getCurrentStageName(holder);
        sInstance->mScenario = holder.mData->getGameDataFile()->getScenarioNo();

        sInstance->mPuppetHolder->setStageInfo(sInstance->mStageName.cstr(), sInstance->mScenario);
    }
}

bool Client::tryAddPuppet(PuppetActor* puppet) {
    if (sInstance) {
        return sInstance->mPuppetHolder->tryRegisterPuppet(puppet);
    } else {
        return false;
    }
}

bool Client::tryAddDebugPuppet(PuppetActor* puppet) {
    if (sInstance) {
        return sInstance->mPuppetHolder->tryRegisterDebugPuppet(puppet);
    } else {
        return false;
    }
}

PuppetActor* Client::getPuppet(int idx) {
    if (sInstance) {
        return sInstance->mPuppetHolder->getPuppetActor(idx);
    } else {
        return nullptr;
    }
}

PuppetInfo* Client::getLatestInfo() {
    if (sInstance) {
        return Client::getPuppetInfo(sInstance->mPuppetHolder->getSize() - 1);
    } else {
        return nullptr;
    }
}

PuppetInfo* Client::getPuppetInfo(int idx) {
    if (sInstance) {
        PuppetInfo* curInfo = sInstance->mPuppetInfoArr[idx];

        if (!curInfo) {
            Logger::log("Attempting to Access Puppet Out of Bounds! Value: %d\n", idx);
            return nullptr;
        }

        return curInfo;
    } else {
        return nullptr;
    }
}

void Client::resetCollectedShines() {
    collectedShineCount = 0;
    curCollectedShines.fill(-1);
}

void Client::removeShine(int shineId) {
    for (size_t i = 0; i < curCollectedShines.size(); i++) {
        if (curCollectedShines[i] == shineId) {
            curCollectedShines[i] = -1;
            collectedShineCount--;
        }
    }
}

bool Client::isNeedUpdateShines() {
    return sInstance ? sInstance->collectedShineCount > 0 : false;
}

/**
 * @brief Processes all pending collected shines on the game thread.
 *        Must only be called when mCurStageScene is valid.
 */
void Client::updateShines() {
    if (!sInstance) {
        Logger::log("Client Null!\n");
        return;
    }

    if (!sInstance->mCurStageScene) {
        Logger::log("updateShines: scene not ready, skipping\n");
        return;
    }

    // skip shine sync if player is in cap kingdom scenario zero (very start of the game)
    if (sInstance->mStageName == "CapWorldHomeStage" && (sInstance->mScenario == 0 || sInstance->mScenario == 1)) {
        return;
    }

    GameDataHolderAccessor accessor(sInstance->mCurStageScene);

    for (size_t i = 0; i < sInstance->getCollectedShinesCount(); i++) {
        int shineID = sInstance->getShineID(i);

        if (shineID < 0)
            continue;

        Logger::log("Shine UID: %d\n", shineID);

        if (shineID >= 2000 && shineID <= 2060) {
            if (!rs::checkGetAchievement(sInstance->mCurStageScene, toadetteMoons[shineID - 2000])) {
                GameDataHolderAccessor(sInstance->mCurStageScene)->getGameDataFile()->getAchievement(toadetteMoons[shineID - 2000]);
            }
            continue;
        }

        GameDataFile::HintInfo* shineInfo = CustomGameDataFunction::getHintInfoByUniqueID(accessor, shineID);

        if (shineInfo) {
            if (!GameDataFunction::isGotShine(accessor, shineInfo->stageName.cstr(), shineInfo->objId.cstr())) {
                Shine* stageShine = findStageShine(shineID);

                if (stageShine) {
                    if (al::isDead(stageShine)) {
                        stageShine->makeActorAlive();
                    }

                    stageShine->getDirect();
                    stageShine->onSwitchGet();
                }

                GameDataHolderAccessor(accessor)->getGameDataFile()->setGotShine(shineInfo);
            }
        }
    }

    sInstance->resetCollectedShines();
    sInstance->mCurStageScene->stageSceneLayout->startShineCountAnim(false);
    sInstance->mCurStageScene->stageSceneLayout->updateCounterParts();
}

/**
 * @brief Main per-frame update. Runs on the game thread.
 *        Drains any coin collects that arrived before the scene was ready.
 */
void Client::update() {
    if (sInstance) {
        sInstance->mPuppetHolder->update();

        if (isNeedUpdateShines()) {
            updateShines();
        }
    }
}

/**
 * @brief Clears puppet, shine, and coin collect arrays. Called on scene exit.
 */
void Client::clearArrays() {
    if (sInstance) {
        sInstance->mPuppetHolder->clearPuppets();
        sInstance->mShineArray.clear();
        sInstance->mCoinCollectArray.clear();
        sInstance->mCoinCollect2DArray.clear();
    }
}

sead::FixedSafeString<MESSAGESIZE>* Client::tryGetMessage() {
    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    MessagePacket* p = (MessagePacket*)mMessageQueue.pop(sead::MessageQueue::BlockType::Blocking);
    sead::FixedSafeString<MESSAGESIZE>* message = new sead::FixedSafeString<MESSAGESIZE>;

    message->append(p->message);
    message->assureTerminationImpl_();

    Logger::log("Getting message with type: %d\n", p->messageType);

    sInstance->mHeap->free(p);

    return message;
}

void Client::setNeedUpdateHealthCoins(bool value) {
    if (!sInstance) {
        return;
    }

    sInstance->mNeedsUpdateHealthCoins = value;
}

void Client::updateHealthCoins(HealthCoins* packet) {
    if (!sInstance) {
        return;
    }

    sInstance->mHealth = packet->health;
    sInstance->mCoins = packet->coins;
    sInstance->isKids = packet->isKids;
    sInstance->mNeedsUpdateHealthCoins = true;
}

void Client::setServerVersion(const char* serverVersion) {
    if (!sInstance) {
        return;
    }

    sInstance->mServerVersion = serverVersion;
}

const char* Client::getServerVersion() {
    if (!sInstance) {
        return "";
    }

    return sInstance->mServerVersion.cstr();
}

PuppetInfo* Client::getDebugPuppetInfo() {
    if (sInstance) {
        return &sInstance->mDebugPuppetInfo;
    } else {
        return nullptr;
    }
}

PuppetActor* Client::getDebugPuppet() {
    if (sInstance) {
        return sInstance->mPuppetHolder->getDebugPuppet();
    } else {
        return nullptr;
    }
}

Keyboard* Client::getKeyboard() {
    if (sInstance) {
        return sInstance->mKeyboard;
    }
    return nullptr;
}

const char* Client::getCurrentIP() {
    if (sInstance) {
        return sInstance->mServerIP.cstr();
    }
    return nullptr;
}

const int Client::getCurrentPort() {
    if (sInstance) {
        return sInstance->mServerPort;
    }
    return -1;
}

const bool Client::hasServerChanged() {
    if (!sInstance) {
        return false;
    }
    return (getCurrentPort() != sInstance->mSocket->getPort() || strcmp(getCurrentIP(), sInstance->mSocket->getIP()) != 0);
}

void Client::setLastUsedIP(const char* ip) {
    if (sInstance) {
        sInstance->mServerIP = ip;
    }
}

void Client::setLastUsedPort(const int port) {
    if (sInstance) {
        sInstance->mServerPort = port;
    }
}

/**
 * @brief Creates new scene info from initInfo and stores a pointer to the current stage scene.
 *        Also clears mCurStageScene first so that any in-flight packets on the read thread
 *        will queue rather than touch a half-initialized scene.
 *
 * @param initInfo
 * @param stageScene
 */
void Client::setSceneInfo(const al::ActorInitInfo& initInfo, const StageScene* stageScene) {
    if (!sInstance) {
        Logger::log("Client Null!\n");
        return;
    }

    // Clear the scene pointer first so the read thread queues packets during the transition
    sInstance->mCurStageScene = nullptr;

    sInstance->mSceneInfo = new al::ActorSceneInfo();
    memcpy(sInstance->mSceneInfo, &initInfo.actorSceneInfo, sizeof(al::ActorSceneInfo));

    sInstance->mCurStageScene = stageScene;
}

bool Client::tryRegisterShine(Shine* shine) {
    if (sInstance) {
        if (!sInstance->mShineArray.isFull()) {
            if (!shine->isGot()) {
                sInstance->mShineArray.pushBack(shine);
                return true;
            }
        }
    }
    return false;
}

void Client::tryRegisterCoinCollect(CoinCollect* coin) {
    if (sInstance) {
        if (!sInstance->mCoinCollectArray.isFull()) {
            sInstance->mCoinCollectArray.pushBack(coin);
        }
    }
}

void Client::tryRegisterCoinCollect2D(CoinCollect2D* coin) {
    if (sInstance) {
        if (!sInstance->mCoinCollect2DArray.isFull()) {
            sInstance->mCoinCollect2DArray.pushBack(coin);
        }
    }
}

Shine* Client::findStageShine(int shineID) {
    if (sInstance) {
        for (int i = 0; i < sInstance->mShineArray.size(); i++) {
            Shine* curShine = sInstance->mShineArray[i];

            if (curShine) {
                auto hintInfo = CustomGameDataFunction::getHintInfoByIndex(curShine, curShine->mShineIdx);

                if (hintInfo->uniqueId == shineID) {
                    return curShine;
                }
            }
        }
    }
    return nullptr;
}

void Client::showConnectError(const char16_t* msg) {
    if (!sInstance)
        return;

    sInstance->mUIMessage->setTxtMessageConfirm(msg);

    al::hidePane(sInstance->mUIMessage, "Page01");

    if (!sInstance->mUIMessage->mIsAlive) {
        sInstance->mUIMessage->appear();

        sInstance->mUIMessage->playLoop();
    }

    al::startAction(sInstance->mUIMessage, "Confirm", "State");
}

void Client::showConnect() {
    if (!sInstance)
        return;

    sInstance->mUIMessage->appear();

    sInstance->mUIMessage->playLoop();
}

void Client::hideConnect() {
    if (!sInstance)
        return;

    sInstance->mUIMessage->tryEnd();
}

/**
 * @brief sends a message packet to the server
 *
 * @param message the message to send
 * @param messageType type of message (0 for chat)
 */
void Client::sendMessagePacket(const char* message, int messageType) {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    if (!message || strlen(message) == 0) {
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    MessagePacket* packet = new MessagePacket();
    packet->mUserID = sInstance->mUserID;
    packet->senderId = sInstance->mUserID;
    packet->messageType = messageType;
    strcpy(packet->message, message);

    sInstance->mSocket->queuePacket(packet);
}