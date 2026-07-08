#include "server/Client.hpp"

#include "hk/diag/diag.h"

#include "nn/os.h"
#include "nn/socket.h"

#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorFlagFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/Play/Layout/SimpleLayoutAppearWaitEnd.h"

#include "game/MapObj/ChangeStageInfo.h"
#include "game/MapObj/CheckpointFlag.h"
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

#include "account.h"
#include "heap/seadHeapMgr.h"
#include "helpers.hpp"
#include "layouts/ConnectionStatus.h"
#include "layouts/PlayerEventLog.h"
#include "layouts/SpeedrunIcon.h"
#include "Library/Base/StringUtil.h"
#include "Library/Layout/LayoutActorUtil.h"
#include "Library/Yaml/ByamlIter.h"
#include "Library/Yaml/ByamlUtil.h"
#include "logger.hpp"
#include "main.hpp"
#include "packets/MoonRockHit.h"
#include "packets/Packet.h"
#include "prim/seadSafeString.h"
#include "puppets/PuppetInfo.h"
#include "Scene/StageScene.h"
#include "Sequence/HakoniwaSequence.h"
#include "server/SocketClient.hpp"
#include "System/GameDataHolder.h"
#include "System/GameDataHolderAccessor.h"
#include "System/GameDataHolderWriter.h"
#include "System/UniqObjInfo.h"
#include "types.h"
#include "Util/AchievementUtil.h"

SEAD_SINGLETON_DISPOSER_IMPL(Client)

/**
 * @brief Construct a new Client:: Client object
 *
 * @param bufferSize defines the maximum amount of puppets the client can handle
 */
Client::Client() {
    sead::ScopedCurrentHeapSetter setter(gHeap);

    mReadThread = new al::AsyncFunctorThread("ClientReadThread", al::FunctorV0M(this, &Client::readFunc), 0,
                                             16_KB, sead::CoreId::cMain);

    mKeyboard = new Keyboard(nn::swkbd::GetRequiredStringBufferSize());

    mSocket = new SocketClient();

    mPuppetHolder = new PuppetHolder(maxPuppets);

    for (size_t i = 0; i < MAXPUPINDEX; i++) {
        mPuppetInfoArr[i] = new PuppetInfo();

        sprintf(mPuppetInfoArr[i]->puppetName, "Puppet%zu", i);
    }

    mConnectCount = 0;

    curCollectedShines.fill(-1);

    collectedShineCount = 0;

    mShineArray.allocBuffer(100, gHeap);  // max of 100 shine actors in buffer

    mCoinCollectArray.allocBuffer(100, gHeap);

    mCoinCollect2DArray.allocBuffer(25, gHeap);

    mPendingCoinCollectCount = 0;

    mPendingCheckpointCount = 0;

    nn::account::GetLastOpenedUser(&mUserID);

    nn::account::Nickname playerName;
    nn::account::GetNickname(&playerName, mUserID);
    Logger::setLogName(playerName.name);  // set Debug logger name to player name

    mUsername = playerName.name;

    mUserID.print();

    hk::diag::logLine("Player Name: %s", playerName.name);

    hk::diag::logLine("%s Build Number: %s", playerName.name, TOSTRING(BUILDVERSTR));
}

/**
 * @brief initializes client class using initInfo obtained from StageScene::init
 *
 * @param initInfo init info used to create layouts used by client
 */
void Client::init(al::LayoutInitInfo const& initInfo, GameDataHolderAccessor holder) {
    sead::ScopedCurrentHeapSetter setter(gHeap);
    if (mUIMessage)
        delete mUIMessage;
    mUIMessage = new al::WindowConfirmWait("ServerWaitConnect", "WindowConfirmWait", initInfo);

    if (mConnectStatus)
        delete mConnectStatus;
    mConnectStatus = new al::SimpleLayoutAppearWaitEnd("", "SaveMessage", initInfo, 0, false);

    if (ConnectionStatus::sInstance)
        delete ConnectionStatus::sInstance;
    ConnectionStatus::sInstance = new ConnectionStatus("Status", initInfo);

    if (SpeedrunIcon::sInstance)
        delete SpeedrunIcon::sInstance;
    SpeedrunIcon::sInstance = new SpeedrunIcon("SpeedrunIcon", initInfo);

    if (PlayerEventLog::instance())
        PlayerEventLog::deleteInstance();
    PlayerEventLog::createInstance(gHeap);

    mUIMessage->setTxtMessage(u"Connecting to Server.");
    mUIMessage->setTxtMessageConfirm(u"Failed to Connect!");

    al::setPaneString(mConnectStatus, "TxtSave", u"Connecting to Server.", 0);
    al::setPaneString(mConnectStatus, "TxtSaveSh", u"Connecting to Server.", 0);

    mHolder = holder;

    startThread();

    // hk::diag::logLine("Heap Free Size: %f/%f", mHeap->getFreeSize() * 0.001f, mHeap->getSize() * 0.001f);
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
        hk::diag::logLine("Read Thread Sucessfully Started.");
        return true;
    } else {
        hk::diag::logLine("Read Thread has already started! Or other unknown reason.");
        return false;
    }
}

void Client::restartConnection() {
    /*if (!sInstance->mIsAllowReconnect)
        return;

    // send disconnect packet
    Packet* dc = new Packet();
    dc->mType = PacketType::PLAYERDC;
    dc->mUserID = Client::getClientId();
    sInstance->mSocket->send(dc);
    delete dc;

    // close socket
    if (sInstance->mSocket->closeSocket()) {
        hk::diag::logLine("Successfully Closed Socket.");
    }

    sInstance->mConnectCount = 0;
    for (PuppetInfo* curInfo : sInstance->mPuppetInfoArr) {
        curInfo->isConnected = false;

        curInfo->scenarioNo = -1;
        strcpy(curInfo->stageName, "");
        curInfo->isInSameStage = false;
    }

    sInstance->mSocket->setLogState(SockState::DISCONNECTED);
    sInstance->mSocket->startEndThread();

    sInstance->mIsConnectionActive =
        sInstance->mSocket->init(sInstance->mServerIP.cstr(), sInstance->mServerPort).IsSuccess();

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
    sInstance->mSocket->send(&sInstance->lastCaptureInfPacket);*/
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
        mKeyboard->setHeaderText(u"IP Address");
        mKeyboard->setSubText(u"Please set a server IP address below.");
        mServerIP = "127.0.0.1";
        Client::openKeyboardIP();
        isNeedSave = true;
    }

    if (!mServerPort || isOverride) {
        mKeyboard->setHeaderText(u"Port");
        mKeyboard->setSubText(u"Please set a server port below.");
        mServerPort = 1027;
        Client::openKeyboardPort();
        isNeedSave = true;
    }

    if (isNeedSave) {
        SaveDataAccessFunction::startSaveDataWrite(mHolder.mData);
    }

    mSocket->init(mServerIP.cstr(), mServerPort);

    while (mSocket->getSocketClientState() == SocketClient::INIT) {
        hk::diag::logLine("log state: %s", mSocket->getStateChar());
        nn::os::YieldThread();
        nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(100000000));
    }

    mIsConnectionActive = mSocket->getLogState() == SockState::CONNECTED;

    if (mIsConnectionActive) {
        hk::diag::logLine("Sucessful Connection. Waiting to recieve init packet.");

        bool waitingForInitPacket = true;

        while (waitingForInitPacket == true) {
            Packet* curPacket = mSocket->tryGetPacket();

            if (curPacket) {
                if (curPacket->mType == PacketType::CLIENTINIT) {
                    InitPacket* initPacket = (InitPacket*)curPacket;

                    hk::diag::logLine("Server Max Player Size: %d", initPacket->maxPlayers);

                    maxPuppets = initPacket->maxPlayers - 1;
                    mPuppetHolder->resizeHolder(maxPuppets);

                    if (al::isStartWithString(initPacket->ServerVersion, "SMOO+")) {
                        sInstance->mIsAllowReconnect = false;
                    } else {
                        sInstance->mIsAllowReconnect = false;
                    }

                    setServerVersion(initPacket->ServerVersion);
                    hk::diag::logLine("Server version: %s", initPacket->ServerVersion);

                    waitingForInitPacket = false;
                }

                delete curPacket;
            } else {
                hk::diag::logLine("Recieve failed! Stopping Connection.");
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
        hk::diag::logLine("Static Instance is null!");
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

    // sInstance->mSocket->setIsFirstConn(isFirstConnect);

    return isFirstConnect;
}

/**
 * @brief Opens up OS's software keyboard in order to change the currently used server port.
 * @returns whether or not a new port has been defined and needs to be saved.
 */
bool Client::openKeyboardPort() {
    if (!sInstance) {
        hk::diag::logLine("Static Instance is null!");
        return false;
    }

    char buf[0x6];
    nn::util::SNPrintf(buf, 0x6, "%u", sInstance->mServerPort);

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

    // sInstance->mSocket->setIsFirstConn(isFirstConnect);

    return isFirstConnect;
}

/**
 * @brief Sets the server IP address
 * @param ip The IP address or hostname to set
 */
void Client::setServerIP(const char* ip) {
    if (!sInstance) {
        hk::diag::logLine("Static Instance is null!");
        return;
    }

    hostname prevIp = sInstance->mServerIP;
    sInstance->mServerIP = ip;

    bool isFirstConnect = prevIp != sInstance->mServerIP;
    // sInstance->mSocket->setIsFirstConn(isFirstConnect);
}

/**
 * @brief Sets the server port
 * @param port The port number to set
 */
void Client::setServerPort(int port) {
    if (!sInstance) {
        hk::diag::log("Static Instance is null!\n");
        return;
    }

    int prevPort = sInstance->mServerPort;
    sInstance->mServerPort = port;

    bool isFirstConnect = prevPort != sInstance->mServerPort;
    // sInstance->mSocket->setIsFirstConn(isFirstConnect);
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
    hk::diag::logLine("Starting Client read thread");

    if (waitForGameInit) {
        nn::os::YieldThread();
        nn::os::SleepThread(nn::TimeSpan::FromSeconds(2));
        waitForGameInit = false;
    }

    mConnectStatus->appear();

    al::startAction(mConnectStatus, "Loop", "Loop");

    if (!startConnection()) {
        hk::diag::logLine("Failed to Connect to Server.");

        nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(250000000));

        mConnectStatus->end();

        return;
    }

    nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(500000000));

    mConnectStatus->end();

    while (mIsConnectionActive) {
        HK_ABORT_UNLESS(mSocket != nullptr, "Client::mSocket was nullptr");
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
            case PacketType::PLAYERDC:
                hk::diag::logLine("Received Player Disconnect!");
                curPacket->mUserID.print();
                disconnectPlayer((PlayerDC*)curPacket);
                break;
            case PacketType::CHANGESTAGE:
                sendToStage((ChangeStagePacket*)curPacket);
                break;
            case PacketType::HEALTHCOINS:
                hk::diag::logLine("Received unused health/coins packet (?)");
                break;
            case PacketType::COINCOLLECTCOLL:
                updateCoinCollects((CoinCollectCollect*)curPacket);
                break;
            case PacketType::CHECKPOINTGET:
                updateCheckpoints((CheckpointGet*)curPacket);
                break;
            case PacketType::MOONROCKHIT:
                updateMoonRocks((MoonRockHit*)curPacket);
                break;

            case PacketType::CLIENTINIT: {
                InitPacket* initPacket = (InitPacket*)curPacket;
                hk::diag::logLine("Server Max Player Size: %d", initPacket->maxPlayers);
                maxPuppets = initPacket->maxPlayers - 1;
                mPuppetHolder->resizeHolder(maxPuppets);
                if (al::isStartWithString(initPacket->ServerVersion, "SMOO+")) {
                    sInstance->mIsAllowReconnect = false;
                } else {
                    sInstance->mIsAllowReconnect = false;
                }
                setServerVersion(initPacket->ServerVersion);
                hk::diag::logLine("Server version: %s", initPacket->ServerVersion);
                break;
            }
            default:
                hk::diag::logLine("Discarding Unknown Packet Type.");
                break;
            }

            delete curPacket;

        } else {
            hk::diag::logLine("SocketClient::tryGetPacket() returned nullptr! Errno: 0x%x",
                              mSocket->socket_errno);
            nn::os::YieldThread();
            nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(100000000));
            // we need to sleep thread to prevent a spin lock when connection is lost
        }
    }

    hk::diag::logLine("Client Read Thread ending.");
}

void Client::sendPlayerInfPacket(const PlayerActorBase* playerBase, bool isYukimaru) {
    sead::ScopedCurrentHeapSetter setter(gHeap);

    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    if (!playerBase) {
        hk::diag::logLine("Error: Null Player Reference");
        return;
    }

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
        delete packet;
    }
}

/**
 * @brief sends info related to player's cap actor to server
 *
 * @param hackCap pointer to cap actor, used to get translation, animation, and state info
 */
void Client::sendHackCapInfPacket(const HackCap* hackCap) {
    sead::ScopedCurrentHeapSetter setter(gHeap);

    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

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

        strncpy(packet->capAnim, al::getActionName(hackCap), sizeof(HackCapInf::capAnim) - 1);
        packet->capAnim[sizeof(HackCapInf::capAnim) - 1] = '\0';

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
    sead::ScopedCurrentHeapSetter setter(gHeap);

    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    GameInf* packet = new GameInf();
    packet->mUserID = sInstance->mUserID;

    if (player) {
        packet->is2D = player->mDimensionKeeper->mIs2D;
    } else {
        packet->is2D = false;
    }

    packet->scenarioNo = holder.mData->getGameDataFile()->getScenarioNo();

    strncpy(packet->stageName, GameDataFunction::getCurrentStageName(holder), sizeof(GameInf::stageName) - 1);
    packet->stageName[sizeof(GameInf::stageName) - 1] = '\0';

    packet->gameMode = -1;

    if (*packet != sInstance->lastGameInfPacket) {
        sInstance->lastGameInfPacket = *packet;
        sInstance->mSocket->queuePacket(packet);
    } else {
        delete packet;
    }
}

/**
 * @brief Sends only stage info to the server.
 * @param holder
 */
void Client::sendGameInfPacket(GameDataHolderAccessor holder, bool isGameStart) {
    sead::ScopedCurrentHeapSetter setter(gHeap);

    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    GameInf* packet = new GameInf();
    packet->mUserID = sInstance->mUserID;

    packet->is2D = false;

    if (isGameStart) {
        packet->scenarioNo = 1;

        strcpy(packet->stageName, "CapWorldHomeStage");
    } else {
        packet->scenarioNo = holder.mData->getGameDataFile()->getScenarioNo();

        strncpy(packet->stageName, GameDataFunction::getCurrentStageName(holder),
                sizeof(GameInf::stageName) - 1);
        packet->stageName[sizeof(GameInf::stageName) - 1] = '\0';
    }

    packet->gameMode = -1;

    packet->isGameStart = isGameStart;

    sInstance->lastGameInfPacket = *packet;

    sInstance->mSocket->queuePacket(packet);
}

/**
 * @brief Sends costume info packet.
 * @param body
 * @param cap
 */
void Client::sendCostumeInfPacket(const char* body, const char* cap) {
    sead::ScopedCurrentHeapSetter setter(gHeap);

    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

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
    sead::ScopedCurrentHeapSetter setter(gHeap);

    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    if (sInstance->isClientCaptured && !sInstance->isSentCaptureInf) {
        CaptureInf* packet = new CaptureInf();
        packet->mUserID = sInstance->mUserID;
        strncpy(packet->hackName, tryConvertName(player->mHackKeeper->getCurrentHackName()),
                sizeof(CaptureInf::hackName) - 1);
        packet->hackName[sizeof(CaptureInf::hackName) - 1] = '\0';
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
    sead::ScopedCurrentHeapSetter setter(gHeap);

    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    if (sInstance->lastCollectedShine != shineID) {
        ShineCollect* packet = new ShineCollect();
        packet->mUserID = sInstance->mUserID;
        packet->shineId = shineID;

        sInstance->lastCollectedShine = shineID;

        sInstance->mSocket->queuePacket(packet);
    }
}

/**
 * @brief Sends coin collect packet.
 * @param placeID
 * @param worldID
 * @param stage
 */
void Client::sendCoinCollectCollectPacket(const char* placeID, int worldID, const char* stage) {
    sead::ScopedCurrentHeapSetter setter(gHeap);

    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    CoinCollectCollect* packet = new CoinCollectCollect();
    packet->mUserID = sInstance->mUserID;
    strncpy(packet->placeID, placeID, sizeof(CoinCollectCollect::placeID) - 1);
    packet->placeID[sizeof(CoinCollectCollect::placeID) - 1] = '\0';
    packet->worldID = worldID;
    strncpy(packet->stage, stage, sizeof(CoinCollectCollect::stage) - 1);
    packet->stage[sizeof(CoinCollectCollect::stage) - 1] = '\0';

    sInstance->mSocket->queuePacket(packet);
}

/**
 * @brief Sends checkpoint get packet.
 * @param objId
 */
void Client::sendCheckpointGetPacket(const char* objId) {
    sead::ScopedCurrentHeapSetter setter(gHeap);

    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    CheckpointGet* packet = new CheckpointGet();
    packet->mUserID = sInstance->mUserID;
    strncpy(packet->objId, objId, sizeof(CheckpointGet::objId) - 1);
    packet->objId[sizeof(CheckpointGet::objId) - 1] = '\0';

    sInstance->mSocket->queuePacket(packet);
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

    if (abs(packet->playerRot.x) > 0.f || abs(packet->playerRot.y) > 0.f || abs(packet->playerRot.z) > 0.f ||
        abs(packet->playerRot.w) > 0.f) {
        if (abs(packet->playerRot.x) <= 1.f || abs(packet->playerRot.y) <= 1.f ||
            abs(packet->playerRot.z) <= 1.f || abs(packet->playerRot.w) <= 1.f) {
            curInfo->playerRot = packet->playerRot;
        }
    }

    if (packet->actName != PlayerAnims::Type::Unknown) {
        strncpy(curInfo->curAnimStr, PlayerAnims::FindStr(packet->actName),
                sizeof(PuppetInfo::curAnimStr) - 1);
        curInfo->curAnimStr[sizeof(PuppetInfo::curAnimStr) - 1] = '\0';
        if (curInfo->curAnimStr[0] == '\0')
            hk::diag::logLine("[ERROR] %s: actName was out of bounds: %d", __func__, packet->actName);
    } else {
        strcpy(curInfo->curAnimStr, "Wait");
    }

    if (packet->subActName != PlayerAnims::Type::Unknown) {
        strncpy(curInfo->curSubAnimStr, PlayerAnims::FindStr(packet->subActName),
                sizeof(PuppetInfo::curSubAnimStr) - 1);
        curInfo->curSubAnimStr[sizeof(PuppetInfo::curSubAnimStr) - 1] = '\0';
        if (curInfo->curSubAnimStr[0] == '\0')
            hk::diag::logLine("[ERROR] %s: subActName was out of bounds: %d", __func__, packet->subActName);
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
        strncpy(curInfo->capAnim, old->capAnim, sizeof(PuppetInfo::capAnim) - 1);
        curInfo->capAnim[sizeof(PuppetInfo::capAnim) - 1] = '\0';
    } else {
        curInfo->capRot = packet->capQuat;
        curInfo->capQuat = packet->capRotQuat;
        curInfo->isCapThrow = packet->isCapVisible;
        strncpy(curInfo->capAnim, packet->capAnim, sizeof(PuppetInfo::capAnim) - 1);
        curInfo->capAnim[sizeof(PuppetInfo::capAnim) - 1] = '\0';
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
        strncpy(curInfo->curHack, packet->hackName, sizeof(PuppetInfo::curHack) - 1);
        curInfo->curHack[sizeof(PuppetInfo::curHack) - 1] = '\0';
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

    strncpy(curInfo->costumeBody, packet->bodyModel, sizeof(PuppetInfo::costumeBody) - 1);
    curInfo->costumeBody[sizeof(PuppetInfo::costumeBody) - 1] = '\0';
    strncpy(curInfo->costumeHead, packet->capModel, sizeof(PuppetInfo::costumeHead) - 1);
    curInfo->costumeHead[sizeof(PuppetInfo::costumeHead) - 1] = '\0';
}

/**
 * @brief Stores shine ID from packet for later processing on the game thread.
 * @param packet
 */
void Client::updateShineInfo(ShineCollect* packet) {
    if (collectedShineCount < curCollectedShines.size() - 1) {
        curCollectedShines[collectedShineCount] = packet->shineId;
        collectedShineCount++;

        if (packet->shineId >= 2000 && packet->shineId <= 2060) {
            PlayerEventLog::addEvent(
                packet->mUserID, PlayerEventLog::SHINE,
                PlayerEventLog::getAchievementMessage(toadetteMoons[packet->shineId - 2000]));
            return;
        }

        GameDataFile::HintInfo* hintInfo =
            CustomGameDataFunction::getHintInfoByUniqueID(mHolder, packet->shineId);
        PlayerEventLog::addEvent(packet->mUserID, PlayerEventLog::SHINE,
                                 PlayerEventLog::getShineMessage(hintInfo->stageName, hintInfo->objId));
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
        hk::diag::logLine("Info is already being used by another connected player!");
        packet->mUserID.print("Connection ID");
        curInfo->playerID.print("Target Info");

    } else {
        packet->mUserID.print("Player Connected! ID");

        curInfo->playerID = packet->mUserID;
        curInfo->isConnected = true;
        strncpy(curInfo->puppetName, packet->clientName, sizeof(PuppetInfo::puppetName) - 1);
        curInfo->puppetName[sizeof(PuppetInfo::puppetName) - 1] = '\0';

        mConnectCount++;

        PlayerEventLog::addEvent(packet->mUserID, PlayerEventLog::CONNECT, "");
    }
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
            strncpy(curInfo->stageName, packet->stageName, sizeof(PuppetInfo::stageName) - 1);
            curInfo->stageName[sizeof(PuppetInfo::stageName) - 1] = '\0';
        }

        curInfo->is2D = packet->is2D;
        curInfo->gameMode = packet->gameMode;

        if (packet->isGameStart) {
            PlayerEventLog::addEvent(packet->mUserID, PlayerEventLog::START, "");
        }
    }
}

/**
 * @brief Sends the player to a new stage as directed by a ChangeStagePacket.
 * @param packet
 */
void Client::sendToStage(ChangeStagePacket* packet) {
    GameDataHolderWriter accessor(mHolder);

    hk::diag::logLine("Sending Player to %s at Entrance %s in Scenario %d", packet->changeStage,
                      packet->changeID, packet->scenarioNo);

    ChangeStageInfo info(accessor.mData, packet->changeID, packet->changeStage, false, packet->scenarioNo,
                         static_cast<ChangeStageInfo::SubScenarioType>(packet->subScenarioType));
    info.setWipeType("FadeBlack");
    GameDataFunction::tryChangeNextStage(accessor, &info);
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

    PlayerEventLog::addEvent(packet->mUserID, PlayerEventLog::DISCONNECT, "");
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
    if (!sInstance)
        return nullptr;

    PuppetInfo* firstAvailable = nullptr;

    for (s32 i = 0; i < getMaxPlayerCount() - 1; i++) {
        PuppetInfo* curInfo = sInstance->mPuppetInfoArr[i];

        if (curInfo->playerID == id) {
            return curInfo;
        } else if (isFindAvailable && !firstAvailable && !curInfo->isConnected) {
            firstAvailable = curInfo;
        }
    }

    if (!firstAvailable) {
        hk::diag::logLine("Unable to find Assigned Puppet for Player!");
        id.print("User ID");
    }

    return firstAvailable;
}

/**
 * @brief Sets the current stage name and scenario number, and updates the puppet holder.
 * @param holder
 */
void Client::setStageInfo(HakoniwaSequence* sequence) {
    if (sInstance) {
        sInstance->mCurStageScene = (StageScene*)sequence->mCurrentScene;
        sInstance->mStageName = sequence->mStageName;
        sInstance->mScenario = sInstance->mHolder->getGameDataFile()->getScenarioNo();

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
            hk::diag::logLine("Attempting to Access Puppet Out of Bounds! Value: %d", idx);
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

struct storyShine {
    s32 uid;
    s32 worldId;
    const char* homeStage;
    s32 scenario;
};

struct moonRock {
    s32 worldId;
    s32 scenario;
};

inline constexpr storyShine scenarioSyncList[16] = {
    {218, 1, "WaterfallWorldHomeStage", 2},  {495, 2, "SandWorldHomeStage", 2},
    {560, 2, "SandWorldHomeStage", 3},       {130, 3, "ForestWorldHomeStage", 2},
    {181, 3, "ForestWorldHomeStage", 3},     {424, 4, "LakeWorldHomeStage", 2},
    {37, 7, "CityWorldHomeStage", 2},        {95, 7, "CityWorldHomeStage", 4},
    {437, 8, "SeaWorldHomeStage", 2},        {1020, 9, "SnowWorldHomeStage", 2},
    {292, 10, "LavaWorldHomeStage", 2},      {290, 10, "LavaWorldHomeStage", 3},
    {795, 11, "BossRaidWorldHomeStage", 2},  {332, 12, "SkyWorldHomeStage", 2},
    {1055, 15, "Special1WorldHomeStage", 2}, {1061, 16, "Special2WorldHomeStage", 2}};

inline constexpr s32 postGameScenarios[14] = {3, 3, 4, 4, 3, 3, 3, 5, 3, 3, 4, 3, 3, 2};

inline constexpr s32 moonRockScenarios[14] = {4, 4, 5, 5, 4, 4, 4, 8, 4, 4, 8, 4, 4, 3};

void Client::updateMoonRocks(MoonRockHit* packet) {
    if (!sInstance)
        return;

    PlayerEventLog::addEvent(packet->mUserID, PlayerEventLog::MOONROCK, worldNames[packet->worldId]);

    if (!GameDataFunction::isGameClear(mHolder)) {
        mPendingMoonRocks[packet->worldId] = true;
        return;
    }

    GameDataFile* gdf = Client::instance()->getHolder()->getGameDataFile();

    if (!gdf) {
        hk::diag::logLine("hitOneMoonRock: GameDataFile null, dropping");
        return;
    }

    // sub scenarios like kfr are reflected in getScenarioNo but not in the scenario array; dont wanna warp
    // someone out of kfr
    bool isSubScenario = gdf->getScenarioNumArr()[packet->worldId] != gdf->getScenarioNo();
    // dont warp if youre already in the scenario
    bool isAlreadyMoonRock = gdf->getScenarioNumArr()[packet->worldId] == moonRockScenarios[packet->worldId];

    gdf->getScenarioNumArr()[packet->worldId] = moonRockScenarios[packet->worldId];

    if (!isSubScenario && !isAlreadyMoonRock &&
        al::isEqualString(homeStageNames[packet->worldId], gdf->getStageNameCurrent())) {
        ChangeStageInfo info(Client::instance()->getHolder(), "MoonRock", gdf->getStageNameCurrent());
        info.mWipeType = "FadeWhite";
        GameDataFunction::tryChangeNextStage(gdf->getGameDataHolder(), &info);
    }
}

void Client::saveMoonRocks(al::ByamlWriter* writer) {
    writer->pushHash("ScenarioSyncMoonRockData");

    for (s32 i = 0; i < SNumMoonRocks; i++) {
        writer->addBool(mPendingMoonRocks[i]);
    }

    writer->pop();
}

void Client::readMoonRocks(const al::ByamlIter& save) {
    al::ByamlIter moonRockIter;
    if (al::tryGetByamlIterByKey(&moonRockIter, save, "ScenarioSyncMoonRockData")) {
        for (s32 i = 0; i < SNumMoonRocks; i++) {
            mPendingMoonRocks[i] = false;
            moonRockIter.tryGetBoolByIndex(&mPendingMoonRocks[i], i);
        }
    }
}

void Client::sendMoonRockHitPacket(int worldId) {
    sead::ScopedCurrentHeapSetter setter(gHeap);

    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    MoonRockHit* packet = new MoonRockHit();
    packet->mUserID = sInstance->mUserID;
    packet->worldId = worldId;

    sInstance->mSocket->queuePacket(packet);
}

/**
 * @brief Processes all pending collected shines on the game thread.
 *        Must only be called when mCurStageScene is valid.
 */
void Client::updateShines() {
    if (!sInstance) {
        hk::diag::logLine("Client Null!");
        return;
    }

    if (!(sInstance->mCurStageScene && gIsSceneAlive)) {
        hk::diag::logLine("updateShines: scene not ready, skipping");
        return;
    }

    // skip shine sync if player is in cap kingdom scenario zero (very start of the game)
    if (sInstance->mStageName == "CapWorldHomeStage" &&
        (sInstance->mScenario == 0 || sInstance->mScenario == 1)) {
        return;
    }

    GameDataHolderAccessor accessor = sInstance->mHolder;

    for (size_t i = 0; i < sInstance->getCollectedShinesCount(); i++) {
        int shineID = sInstance->getShineID(i);

        if (shineID < 0)
            continue;

        hk::diag::logLine("Shine UID: %d", shineID);

        for (const storyShine& shine : scenarioSyncList) {
            if (shine.uid == shineID) {
                s32& scenarioNum =
                    Client::instance()->getHolder()->getGameDataFile()->getScenarioNumArr()[shine.worldId];

                if (scenarioNum < shine.scenario) {
                    bool isSubScenario = scenarioNum != accessor->getGameDataFile()->getScenarioNo();

                    scenarioNum = shine.scenario;

                    if (!isSubScenario &&
                        al::isEqualString(shine.homeStage, GameDataFunction::getCurrentStageName(accessor))) {
                        ChangeStageInfo info(accessor, "start",
                                             GameDataFunction::getCurrentStageName(accessor));
                        info.mWipeType = "FadeWhite";
                        // avoid setting mIsStageChanging because we want to allow checkpoint warps to
                        // override the scenario sync warp. this is why we aren't using
                        // GameDataFunction::tryChangeNextStage()
                        if (!accessor->mIsStageChanging) {
                            accessor->getGameDataFile()->changeNextStage(&info, 0);
                            accessor->resetLocationName();
                        }
                    }
                }

                break;
            }
        }

        if (shineID >= 2000 && shineID <= 2060) {
            if (!rs::checkGetAchievement(sInstance->mCurStageScene, toadetteMoons[shineID - 2000])) {
                sInstance->getHolder()->getGameDataFile()->getAchievement(toadetteMoons[shineID - 2000]);
            }
            continue;
        }

        GameDataFile::HintInfo* shineInfo = CustomGameDataFunction::getHintInfoByUniqueID(accessor, shineID);

        if (shineInfo) {
            if (!GameDataFunction::isGotShine(accessor, shineInfo->stageName.cstr(),
                                              shineInfo->objId.cstr())) {
                Shine* stageShine = findStageShine(shineID);

                if (stageShine) {
                    if (al::isDead(stageShine)) {
                        stageShine->makeActorAlive();
                    }

                    stageShine->getDirect();
                    stageShine->onSwitchGet();
                }

                accessor->getGameDataFile()->setGotShine(shineInfo);
            }
        }
    }

    sInstance->resetCollectedShines();
    sInstance->mCurStageScene->stageSceneLayout->startShineCountAnim(false);
    sInstance->mCurStageScene->stageSceneLayout->updateCounterParts();
}

/**
 * @brief Core logic for applying a received coin collect to game state and killing the actor.
 *        Called from updateCoinCollects() when the scene is ready, or from update() when
 *        draining the pending queue.
 *
 * @param placeID placement ID string of the coin collect
 * @param worldID world ID
 * @param stage   stage name string
 */
void Client::applyOneCoinCollect(const char* placeID, int worldID, const char* stage) {
    al::PlacementId pid(placeID, nullptr, nullptr);
    GameDataFile* gdf = sInstance->getHolder()->getGameDataFile();
    if (!gdf) {
        hk::diag::logLine("applyOneCoinCollect: GameDataFile null, dropping");
        return;
    }

    gdf->customAddCoinCollect(&pid, worldID, stage);

    if (gdf->isGotCoinCollect(&pid)) {
        for (s32 i = 0; i < sInstance->mCoinCollectArray.size(); i++) {
            if (sInstance->mCoinCollectArray[i] && sInstance->mCoinCollectArray[i]->mPlacementId &&
                sInstance->mCoinCollectArray[i]->mPlacementId->isEqual(pid)) {
                sInstance->mCoinCollectArray[i]->makeActorDead();
                return;
            }
        }
        for (s32 i = 0; i < sInstance->mCoinCollect2DArray.size(); i++) {
            if (sInstance->mCoinCollect2DArray[i] && sInstance->mCoinCollect2DArray[i]->mPlacementId &&
                sInstance->mCoinCollect2DArray[i]->mPlacementId->isEqual(pid)) {
                sInstance->mCoinCollect2DArray[i]->makeActorDead();
                return;
            }
        }
    }
}

/**
 * @brief Receives a coin collect packet from the read thread.
 *        If the scene is ready, applies immediately. Otherwise queues it for update().
 *
 * @param packet
 */
void Client::updateCoinCollects(CoinCollectCollect* packet) {
    if (!sInstance) {
        return;
    }

    PlayerEventLog::addEvent(packet->mUserID, PlayerEventLog::PURPLE, worldNames[packet->worldID]);

    if (!(sInstance->mCurStageScene && gIsSceneAlive)) {
        // Scene not ready — queue for later processing in update()
        if (sInstance->mPendingCoinCollectCount < sMaxPendingCoinCollects) {
            PendingCoinCollect& pending =
                sInstance->mPendingCoinCollects[sInstance->mPendingCoinCollectCount++];
            strncpy(pending.placeID, packet->placeID, sizeof(PendingCoinCollect::placeID) - 1);
            pending.placeID[sizeof(PendingCoinCollect::placeID) - 1] = '\0';
            pending.worldID = packet->worldID;
            strncpy(pending.stage, packet->stage, sizeof(PendingCoinCollect::stage) - 1);
            pending.stage[sizeof(PendingCoinCollect::stage) - 1] = '\0';
            hk::diag::logLine("updateCoinCollects: scene not ready, queued (total pending: %d)",
                              sInstance->mPendingCoinCollectCount);
        } else {
            hk::diag::logLine("updateCoinCollects: pending queue full, dropping packet");
        }
        return;
    }

    applyOneCoinCollect(packet->placeID, packet->worldID, packet->stage);
}

/**
 * @brief Core logic for marking a checkpoint as collected and warpable when a checkpoint get packet is
 * received from the read thread.
 *
 * @param objId
 */
void Client::getOneCheckpoint(const char* objId) {
    GameDataFile* gdf = sInstance->getHolder()->getGameDataFile();

    if (!gdf) {
        hk::diag::logLine("updateCheckpoints: GameDataFile null, dropping");
        return;
    }

    al::PlacementId placeId(objId, nullptr, nullptr);
    UniqObjInfo* info = gdf->customSetCheckpointId(&placeId);
    if (info && al::isEqualString(info->getStageName(), sInstance->mStageName.cstr())) {
        CheckpointFlag* checkpoint = rs::tryFindCheckpointFlag(sInstance->mCurStageScene, objId);
        if (checkpoint) {
            al::startHitReaction(checkpoint, "取得");
            al::startAction(checkpoint, "Get");
            rs::requestHideCheckpointFlagBalloon(checkpoint);
            checkpoint->getCheckpoint();
            checkpoint->setAfter();
        }
    }
}

/**
 * @brief Receives a checkpoint get packet from the read thread.
 *
 * @param packet
 */
void Client::updateCheckpoints(CheckpointGet* packet) {
    if (!sInstance)
        return;

    PlayerEventLog::addEvent(packet->mUserID, PlayerEventLog::CHECKPOINT,
                             PlayerEventLog::getCheckpointMessage(packet->objId));

    if (!(sInstance->mCurStageScene && gIsSceneAlive)) {
        if (sInstance->mPendingCheckpointCount < sMaxPendingCheckpoints) {
            PendingCheckpoint& pending = sInstance->mPendingCheckpoints[sInstance->mPendingCheckpointCount++];
            strncpy(pending.objId, packet->objId, sizeof(PendingCheckpoint::objId) - 1);
            pending.objId[sizeof(PendingCheckpoint::objId) - 1] = '\0';
            hk::diag::logLine("updateCheckpoints: scene not ready, queued (total pending: %d)",
                              sInstance->mPendingCoinCollectCount);
        } else {
            hk::diag::logLine("updateCheckpoints: pending queue full, dropping packet");
        }
        return;
    }

    getOneCheckpoint(packet->objId);
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

        if (sInstance->mCurStageScene && gIsSceneAlive) {
            // Drain coin collects that arrived while the scene was loading
            if (sInstance->mPendingCoinCollectCount > 0) {
                hk::diag::logLine("update: draining %d pending coin collect(s)",
                                  sInstance->mPendingCoinCollectCount);
                for (s32 i = 0; i < sInstance->mPendingCoinCollectCount; i++) {
                    PendingCoinCollect& p = sInstance->mPendingCoinCollects[i];
                    applyOneCoinCollect(p.placeID, p.worldID, p.stage);
                }
                sInstance->mPendingCoinCollectCount = 0;
            }

            // Drain checkpoints that arrived while the scene was loading
            if (sInstance->mPendingCheckpointCount > 0) {
                hk::diag::log("update: draining %d pending checkpoint(s)\n",
                              sInstance->mPendingCheckpointCount);
                for (s32 i = 0; i < sInstance->mPendingCheckpointCount; i++) {
                    PendingCheckpoint& c = sInstance->mPendingCheckpoints[i];
                    getOneCheckpoint(c.objId);
                }
                sInstance->mPendingCheckpointCount = 0;
            }
        }

        if (GameDataFunction::isGameClear(Client::instance()->getHolder())) {
            // Set moon rock scenarios for rocks that were hit prior to the player beating the game
            for (s32 i = 0; i < SNumMoonRocks; i++) {
                if (Client::instance()->mPendingMoonRocks[i]) {
                    Client::instance()->getHolder()->getGameDataFile()->getScenarioNumArr()[i] =
                        moonRockScenarios[i];
                }
            }
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

/*void Client::setNeedUpdateHealthCoins(bool value) {
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
}*/

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

bool Client::hasServerChanged() {
    if (!sInstance) {
        return false;
    }
    hk::diag::logLine("client port: %d\n socket port: %d\n client ip: %s\n socket ip: %s", getCurrentPort(),
                      sInstance->mSocket->getPort(), getCurrentIP(), sInstance->mSocket->getIP());
    return (getCurrentPort() != sInstance->mSocket->getPort() ||
            !al::isEqualString(getCurrentIP(), sInstance->mSocket->getIP()));
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
