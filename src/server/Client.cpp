#include "server/Client.hpp"

#include "hk/types.h"
#include "hk/util/Math.h"

#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Layout/LayoutActionFunction.h"
#include "al/Library/LiveActor/ActorActionFunction.h"
#include "al/Library/LiveActor/ActorFlagFunction.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/Play/Layout/SimpleLayoutAppearWaitEnd.h"

#include "game/Player/HackCap.h"
#include "game/Player/PlayerAnimator.h"
#include "game/Player/PlayerAnimFrameCtrl.h"
#include "game/Player/PlayerHackKeeper.h"
#include "game/Sequence/ChangeStageInfo.h"
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
#include "Library/LiveActor/LiveActor.h"
#include "logger.hpp"
#include "nn/os.h"
#include "nn/socket.h"
#include "packets/Packet.h"
#include "server/freeze/FreezeTagMode.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/hns/HideAndSeekMode.hpp"
#include "server/shine-thief/ShineThiefInfo.h"
#include "server/shine-thief/ShineThiefMode.hpp"
#include "server/snh/SardineMode.hpp"
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
    // Just close the socket without sending disconnect packet
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
        // Assume game packets are empty from first connection
        if (sInstance->lastGameInfPacket.mUserID != sInstance->mUserID) {
            sInstance->lastGameInfPacket.mUserID = sInstance->mUserID;
        }
        sInstance->mSocket->send(&sInstance->lastGameInfPacket);
    }

    // No need to send player/costume packets if they're empty
    if (sInstance->lastPlayerInfPacket.mUserID == sInstance->mUserID) {
        sInstance->mSocket->send(&sInstance->lastPlayerInfPacket);
    }

    if (sInstance->lastCostumeInfPacket.mUserID == sInstance->mUserID) {
        sInstance->mSocket->send(&sInstance->lastCostumeInfPacket);
    }

    if (sInstance->lastCaptureInfPacket.mUserID == sInstance->mUserID) {
        sInstance->mSocket->send(&sInstance->lastCaptureInfPacket);
    }
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
        // wait for client init packet

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

    // opens swkbd with the initial text set to the last saved IP
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
        nn::os::YieldThread();  // allow other threads to run
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

    // opens swkbd with the initial text set to the last saved port
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
        nn::os::YieldThread();  // allow other threads to run
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
        nn::os::YieldThread();  // sleep the thread for the first thing we do so that game init can
                                // finish
        nn::os::SleepThread(nn::TimeSpan::FromSeconds(2));
        waitForGameInit = false;
    }

    mConnectStatus->appear();

    al::startAction(mConnectStatus, "Loop", "Loop");

    if (!startConnection()) {
        Logger::log("Failed to Connect to Server.\n");

        nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(250000000));  // sleep active thread for 0.25 seconds

        mConnectStatus->end();

        return;
    }

    nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(500000000));  // sleep for 0.5 seconds to let connection layout fully show
                                                                    // (probably should find a better way to do this)

    mConnectStatus->end();

    while (mIsConnectionActive) {
        Packet* curPacket = mSocket->tryGetPacket();  // will block until a packet has been
                                                      // recieved, or socket disconnected

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

                // Send relevant info packets when another client is connected

                if (lastGameInfPacket != emptyGameInfPacket) {
                    // Assume game packets are empty from first connection
                    if (lastGameInfPacket.mUserID != mUserID) {
                        lastGameInfPacket.mUserID = mUserID;
                    }
                    mSocket->send(&lastGameInfPacket);
                }

                // No need to send player/costume packets if they're empty
                if (lastPlayerInfPacket.mUserID == mUserID) {
                    mSocket->send(&lastPlayerInfPacket);
                }
                if (lastCostumeInfPacket.mUserID == mUserID) {
                    mSocket->send(&lastCostumeInfPacket);
                }

                if (lastCaptureInfPacket.mUserID == mUserID) {
                    mSocket->send(&lastCaptureInfPacket);
                }

                if (GameModeManager::instance()->isMode(GameMode::SHINETHIEF)) {
                    ShineThiefInfo* stInfo = GameModeManager::instance()->getInfo<ShineThiefInfo>();
                    ShineThiefMode* stMode = GameModeManager::instance()->getMode<ShineThiefMode>();

                    if (stInfo && stMode) {
                        ShineThiefInf* stPacket = new (mHeap) ShineThiefInf();
                        stPacket->mUserID = mUserID;
                        stPacket->updateType = ShineThiefUpdateType::PLAYER;
                        stPacket->isHolder = stInfo->mIsPlayerHolder;
                        stPacket->isCaught = false;
                        stPacket->score = stInfo->mPlayerTagScore.mScore;
                        stPacket->shinePos = stMode->getShinePos();

                        switch (stInfo->mPlayerTeam) {
                        case ShineThiefTeam::TEAM_1:
                            stPacket->team = 1;
                            break;
                        case ShineThiefTeam::TEAM_2:
                            stPacket->team = 2;
                            break;
                        default:
                            stPacket->team = 0;
                            break;
                        }

                        mSocket->send(stPacket);
                        mHeap->free(stPacket);
                    }
                }

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
            case PacketType::COINCOLLECTCOLL:
                updateCoinCollects((CoinCollectCollect*)curPacket);
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

        } else {  // if false, socket has errored or disconnected, so close the socket and end this
                  // thread.
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

    // if cap is in flying state, send packet as often as this function is called
    if (isFlying) {
        HackCapInf* packet = new HackCapInf();
        packet->mUserID = sInstance->mUserID;
        packet->capPos = al::getTrans(hackCap);

        packet->isCapVisible = isFlying;

        // Joint rotations, skew & quat
        packet->capQuat.x = hackCap->mJointKeeper->mJointRot.x;
        packet->capQuat.y = hackCap->mJointKeeper->mJointRot.y;
        packet->capQuat.z = hackCap->mJointKeeper->mJointRot.z;
        packet->capQuat.w = hackCap->mJointKeeper->mSkew;
        packet->capRotQuat = al::getQuat(hackCap);

        strcpy(packet->capAnim, al::getActionName(hackCap));

        sInstance->mSocket->queuePacket(packet);

        sInstance->isSentHackInf = true;

    } else if (sInstance->isSentHackInf) {  // if cap is not flying, check to see if previous
                                            // function call sent a packet, and if so, send one
                                            // final packet resetting cap data.
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
 * @brief
 * Sends both stage info and player 2D info to the server.
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

    GameModeManager* gmm = GameModeManager::instance();
    packet->gameMode = gmm ? static_cast<s8>(gmm->getGameMode()) : static_cast<s8>(-1);

    if (*packet != sInstance->lastGameInfPacket) {
        sInstance->lastGameInfPacket = *packet;
        sInstance->mSocket->queuePacket(packet);
    } else {
        sInstance->mHeap->free(packet);  // free packet if we're not using it
    }
}
/**
 * @brief
 * Sends only stage info to the server.
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

    GameModeManager* gmm = GameModeManager::instance();
    packet->gameMode = gmm ? static_cast<s8>(gmm->getGameMode()) : static_cast<s8>(-1);

    sInstance->lastGameInfPacket = *packet;

    sInstance->mSocket->queuePacket(packet);
}

/**
 * @brief
 *
 */
void Client::sendTagInfPacket() {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    GameMode curMode = GameModeManager::instance()->getGameMode();
    HideAndSeekMode* hsMode;
    HideAndSeekInfo* hsInfo;
    SardineMode* sarMode;
    SardineInfo* sarInfo;

    switch (GameModeManager::instance()->getGameMode()) {
    case GameMode::HIDEANDSEEK:
        hsMode = GameModeManager::instance()->getMode<HideAndSeekMode>();
        hsInfo = GameModeManager::instance()->getInfo<HideAndSeekInfo>();
        break;
    case GameMode::SARDINE:
        sarMode = GameModeManager::instance()->getMode<SardineMode>();
        sarInfo = GameModeManager::instance()->getInfo<SardineInfo>();
        break;
    case GameMode::NONE:
        Logger::log("Tag info packet has unknown gamemode!\n");
        return;
    default:
        Logger::log("Tag info packet has unknown gamemode!\n");
        return;
    };

    TagInf* packet = new TagInf();

    packet->mUserID = sInstance->mUserID;

    if (curMode == GameMode::HIDEANDSEEK) {
        packet->isIt = hsMode->isPlayerIt();
        packet->minutes = hsInfo->mHidingTime.mMinutes;
        packet->seconds = hsInfo->mHidingTime.mSeconds;
    } else if (curMode == GameMode::SARDINE) {
        packet->isIt = sarMode->isPlayerIt();
        packet->minutes = sarInfo->mHidingTime.mMinutes;
        packet->seconds = sarInfo->mHidingTime.mSeconds;
    }

    packet->updateType = static_cast<TagUpdateType>(TagUpdateType::STATE | TagUpdateType::TIME);

    sInstance->mSocket->queuePacket(packet);
}

void Client::sendFreezeInfPacket() {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    GameMode curMode = GameModeManager::instance()->getGameMode();
    if (curMode != GameMode::FREEZETAG) {
        Logger::log("Attempting to send FreezeInf packet while not in Freeze Tag mode!\n");
        return;
    }

    FreezeTagMode* frMode = GameModeManager::instance()->getMode<FreezeTagMode>();
    FreezeTagInfo* frInfo = GameModeManager::instance()->getInfo<FreezeTagInfo>();

    FreezeUpdateType updateType = frMode->getNextUpdateType();

    // Send round packet for round-related updates
    if (updateType == FreezeUpdateType::ROUNDSTART || updateType == FreezeUpdateType::ROUNDCANCEL) {
        FreezeInfRoundPacket* packet = new FreezeInfRoundPacket();

        packet->mUserID = sInstance->mUserID;
        packet->updateType = updateType;

        if (updateType == FreezeUpdateType::ROUNDSTART) {
            packet->roundTime = frInfo->mRoundLength;  // Make sure this field exists in FreezeTagInfo
        }

        sInstance->mSocket->queuePacket(packet);
    }
    // Send regular packet for player updates
    else {
        FreezeInf* packet = new FreezeInf();

        packet->mUserID = sInstance->mUserID;
        packet->updateType = updateType;
        packet->isRunner = frInfo->mIsPlayerRunner;
        packet->isFreeze = frInfo->mIsPlayerFreeze;
        packet->score = frInfo->mPlayerTagScore.mScore;

        sInstance->mSocket->queuePacket(packet);
    }
}

void Client::sendShineThiefInfPacket() {
    if (!sInstance) {
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    GameMode curMode = GameModeManager::instance()->getGameMode();
    if (curMode != GameMode::SHINETHIEF) {
        return;
    }

    ShineThiefMode* stMode = GameModeManager::instance()->getMode<ShineThiefMode>();
    ShineThiefInfo* stInfo = GameModeManager::instance()->getInfo<ShineThiefInfo>();

    if (!stMode || !stInfo) {
        return;
    }

    ShineThiefUpdateType updateType = stMode->getNextUpdateType();

    // Round packets
    if (updateType == ShineThiefUpdateType::ROUNDSTART || updateType == ShineThiefUpdateType::ROUNDCANCEL) {
        ShineThiefInfRoundPacket* packet = new ShineThiefInfRoundPacket();

        packet->mUserID = sInstance->mUserID;
        packet->updateType = updateType;

        if (updateType == ShineThiefUpdateType::ROUNDSTART) {
            packet->roundTime = stInfo->mRoundLength;
            packet->shinePos = stMode->getShinePos();
            packet->hostStartPos = stMode->getHostStartPos();
        }

        sInstance->mSocket->queuePacket(packet);
    }
    // Regular player update packets
    else {
        ShineThiefInf* packet = new ShineThiefInf();

        packet->mUserID = sInstance->mUserID;
        packet->updateType = updateType;
        packet->isHolder = stInfo->mIsPlayerHolder;
        packet->isCaught = false;
        packet->score = stInfo->mPlayerTagScore.mScore;
        packet->shinePos = stMode->getShinePos();

        // Set team explicitly
        switch (stInfo->mPlayerTeam) {
        case ShineThiefTeam::TEAM_1:
            packet->team = 1;
            break;
        case ShineThiefTeam::TEAM_2:
            packet->team = 2;
            break;
        default:
            packet->team = 0;
            break;
        }

        sInstance->mSocket->queuePacket(packet);
    }
}

/**
 * @brief
 *
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
 * @brief
 *
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
 * @brief
 *
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
 * @brief
 *
 * @param placeID
 * @param worldID
 * @param stage
 */
void Client::sendCoinCollectCollectPacket(const char* placeID, int worldID, const char* stage) {
    if (!sInstance) {
        Logger::log("Static Instance is Null!\n");
        return;
    }

    sead::ScopedCurrentHeapSetter setter(sInstance->mHeap);

    CoinCollectCollect* packet = new CoinCollectCollect();
    packet->mUserID = sInstance->mUserID;
    strcpy(packet->placeID, placeID);
    packet->worldID = worldID;
    strcpy(packet->stage, stage);

    sInstance->mSocket->queuePacket(packet);
}

/**
 * @brief
 *
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

    // check if rotation is larger than zero and less than or equal to 1
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
        // weights can only be between 0 and 1
        if (packet->animBlendWeights[i] >= 0.f && packet->animBlendWeights[i] <= 1.f) {
            curInfo->blendWeights[i] = packet->animBlendWeights[i];
        }
    }

    // TEMP

    if (!curInfo->isCapThrow) {
        curInfo->capPos = packet->playerPos;
    }
}

/**
 * @brief
 *
 * @param packet
 */
void Client::updateHackCapInfo(HackCapInf* packet) {
    PuppetInfo* curInfo = findPuppetInfo(packet->mUserID, false);

    if (curInfo) {
        curInfo->capPos = packet->capPos;
        curInfo->capRot = packet->capQuat;
        curInfo->capQuat = packet->capRotQuat;

        curInfo->isCapThrow = packet->isCapVisible;

        strcpy(curInfo->capAnim, packet->capAnim);
    }
}

/**
 * @brief
 *
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
 * @brief
 *
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
 * @brief
 *
 * @param packet
 */
void Client::updateShineInfo(ShineCollect* packet) {
    if (collectedShineCount < curCollectedShines.size() - 1) {
        curCollectedShines[collectedShineCount] = packet->shineId;
        collectedShineCount++;
    }
}

/**
 * @brief
 *
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
    const char* warpStage = stage;
} stageListForScenarioSync[] = {{"CapWorldHomeStage", 0},
                                {"WaterfallWorldHomeStage", 1},
                                {"SandWorldHomeStage", 2},
                                {"SandWorldUnderground001Stage", 2, "SandWorldHomeStage"},
                                {"ForestWorldHomeStage", 3},
                                {"ForestWorldBossStage", 3, "ForestWorldHomeStage"},
                                {"LakeWorldHomeStage", 4},
                                {"CloudWorldHomeStage", 5},
                                {"ClashWorldHomeStage", 6},
                                {"CityWorldHomeStage", 7},
                                {"SeaWorldHomeStage", 8},
                                {"SnowWorldHomeStage", 9},
                                {"SnowWorldLobby001Stage", 9, "SnowWorldHomeStage"},
                                {"LavaWorldHomeStage", 10},
                                {"BossRaidWorldHomeStage", 11},
                                {"SkyWorldHomeStage", 12},
                                {"PeachWorldHomeStage", 13},
                                {"Special1WorldHomeStage", 14},
                                {"Special2WorldHomeStage", 15}};
static s32 findWorldIdFromStageName(const char* stageName) {
    for (s32 i = 0; i < hk::util::arraySize(stageListForScenarioSync); i++) {
        if (al::isEqualString(stageListForScenarioSync[i].stage, stageName)) {
            return stageListForScenarioSync[i].index;
        }
    }
    return -1;
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
 * @brief
 *
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
    int scenario = Client::sInstance->getHolder()->getGameDataFile()->getScenarioNumArr()[findWorldIdFromStageName(packet->stageName)];
    // int mainscenario = Client::sInstance->getHolder()->getGameDataFile()->getMainScenarioNumArr()[findWorldIdFromStageName(packet->stageName)];
    if (packet->scenarioNo < 15 && packet->scenarioNo > scenario) {
        Client::sInstance->getHolder()->getGameDataFile()->getScenarioNumArr()[findWorldIdFromStageName(packet->stageName)] = packet->scenarioNo;
        // Client::sInstance->getHolder()->getGameDataFile()->getMainScenarioNumArr()[findWorldIdFromStageName(packet->stageName)] = packet->scenarioNo;
        if (findWarpStageFromStageName(packet->stageName) &&
            strcmp(GameDataFunction::getCurrentStageName(Client::getHolder()), findWarpStageFromStageName(packet->stageName)) == 0) {
            ChangeStageInfo info(Client::getHolder(), "start", findWarpStageFromStageName(packet->stageName), false, packet->scenarioNo);
            Client::getHolder()->changeNextStage(&info);
        }
    }
}

/**
 * @brief
 *
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
 * @brief
 *
 * @param packet
 */
void Client::updateTagInfo(TagInf* packet) {
    GameMode mode = GameModeManager::instance()->getGameMode();

    if (mode == GameMode::HIDEANDSEEK || mode == GameMode::SARDINE) {
        // if the packet is for our player, edit info for our player
        if (packet->mUserID == mUserID && GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK)) {
            HideAndSeekMode* mMode = GameModeManager::instance()->getMode<HideAndSeekMode>();
            HideAndSeekInfo* curInfo = GameModeManager::instance()->getInfo<HideAndSeekInfo>();

            if (packet->updateType & TagUpdateType::STATE) {
                mMode->setPlayerTagState(packet->isIt);
            }

            if (packet->updateType & TagUpdateType::TIME) {
                curInfo->mHidingTime.mSeconds = packet->seconds;
                curInfo->mHidingTime.mMinutes = packet->minutes;
            }

            return;
        }

        if (packet->mUserID == mUserID && GameModeManager::instance()->isMode(GameMode::SARDINE)) {
            SardineMode* mMode = GameModeManager::instance()->getMode<SardineMode>();
            SardineInfo* curInfo = GameModeManager::instance()->getInfo<SardineInfo>();

            if (packet->updateType & TagUpdateType::STATE) {
                mMode->setPlayerTagState(packet->isIt);
            }

            if (packet->updateType & TagUpdateType::TIME) {
                curInfo->mHidingTime.mSeconds = packet->seconds;
                curInfo->mHidingTime.mMinutes = packet->minutes;
            }

            return;
        }

        PuppetInfo* curInfo = findPuppetInfo(packet->mUserID, false);

        if (!curInfo) {
            return;
        }

        curInfo->isIt = packet->isIt;
        curInfo->seconds = packet->seconds;
        curInfo->minutes = packet->minutes;
    }

    if (mode == GameMode::FREEZETAG) {
        FreezeInf* freezePak = (FreezeInf*)packet;

        // Handle round packets first
        if (freezePak->updateType == FreezeUpdateType::ROUNDSTART || freezePak->updateType == FreezeUpdateType::ROUNDCANCEL) {
            FreezeInfRoundPacket* roundPak = (FreezeInfRoundPacket*)packet;
            FreezeTagMode* mMode = GameModeManager::instance()->getMode<FreezeTagMode>();

            if (roundPak->updateType == FreezeUpdateType::ROUNDSTART) {
                if (mMode) {
                    mMode->startRound(roundPak->roundTime);
                }
            } else if (roundPak->updateType == FreezeUpdateType::ROUNDCANCEL) {
                if (mMode) {
                    mMode->endRound(true);
                }
            }
            return;  // Don't process as regular freeze packet
        }

        // Handle regular freeze packets
        if (packet->mUserID == mUserID && GameModeManager::instance()->isMode(GameMode::FREEZETAG)) {
            FreezeTagMode* mMode = GameModeManager::instance()->getMode<FreezeTagMode>();
            FreezeTagInfo* curInfo = GameModeManager::instance()->getInfo<FreezeTagInfo>();

            curInfo->mIsPlayerRunner = freezePak->isRunner;

            if (freezePak->isFreeze && !freezePak->isRunner)
                mMode->trySetPlayerRunnerState(FreezeState::FREEZE);
            else
                mMode->trySetPlayerRunnerState(FreezeState::ALIVE);

            return;
        }

        PuppetInfo* curInfo = findPuppetInfo(packet->mUserID, false);

        if (!curInfo)
            return;

        if (!GameModeManager::instance()->isActive()) {
            curInfo->isFreezeTagFreeze = freezePak->isFreeze;
            curInfo->isFreezeTagRunner = freezePak->isRunner;
            curInfo->freezeTagScore = freezePak->score;
            return;
        }

        FreezeTagMode* mMode = GameModeManager::instance()->getMode<FreezeTagMode>();

        if (mMode->isScoreEventsEnabled())
            mMode->tryScoreEvent(freezePak, curInfo);

        curInfo->isFreezeTagFreeze = freezePak->isFreeze;
        curInfo->isFreezeTagRunner = freezePak->isRunner;
        curInfo->freezeTagScore = freezePak->score;
    }

    if (mode == GameMode::SHINETHIEF) {
        ShineThiefInf* shinePak = (ShineThiefInf*)packet;

        // Handle round packets
        if (GameModeManager::instance()->isActive() &&
            (shinePak->updateType == ShineThiefUpdateType::ROUNDSTART || shinePak->updateType == ShineThiefUpdateType::ROUNDCANCEL)) {
            ShineThiefInfRoundPacket* roundPak = (ShineThiefInfRoundPacket*)packet;
            ShineThiefMode* mMode = GameModeManager::instance()->getMode<ShineThiefMode>();

            if (roundPak->updateType == ShineThiefUpdateType::ROUNDSTART) {
                if (mMode && !mMode->isPlayerHolder()) {
                    mMode->setHostStartPos(roundPak->hostStartPos);
                    mMode->setShinePos(roundPak->shinePos);
                    mMode->startRound(roundPak->roundTime);
                }
            } else if (roundPak->updateType == ShineThiefUpdateType::ROUNDCANCEL) {
                if (mMode)
                    mMode->endRound(true);
            }
            return;
        }

        // Ignore own packets
        if (packet->mUserID == mUserID)
            return;

        PuppetInfo* curPupInfo = findPuppetInfo(packet->mUserID, false);
        if (!curPupInfo)
            return;

        bool puppetIsNowHolder = shinePak->isHolder;
        bool puppetWasHolder = curPupInfo->isShineThiefHolder;

        // Update if mode inactive
        if (!GameModeManager::instance()->isActive()) {
            curPupInfo->isShineThiefHolder = puppetIsNowHolder;
            curPupInfo->shineThiefScore = shinePak->score;
            curPupInfo->shineThiefTeam = shinePak->team;
            return;
        }

        ShineThiefMode* mMode = GameModeManager::instance()->getMode<ShineThiefMode>();

        // Enforce single holder
        if (mMode && puppetIsNowHolder && !puppetWasHolder && mMode->isPlayerHolder())
            mMode->forceDropShine();

        // Score events
        if (mMode && mMode->isScoreEventsEnabled()) {
            if (!puppetWasHolder && puppetIsNowHolder && !mMode->isPlayerHolder())
                mMode->tryScoreEvent(shinePak, curPupInfo);
            if (puppetWasHolder && !puppetIsNowHolder && !mMode->isPlayerHolder())
                mMode->tryScoreEvent(shinePak, curPupInfo);
        }

        // Update puppet state
        curPupInfo->isShineThiefHolder = puppetIsNowHolder;
        curPupInfo->shineThiefScore = shinePak->score;
        curPupInfo->shineThiefTeam = shinePak->team;

        // Handle FALLOFF
        if (shinePak->updateType == ShineThiefUpdateType::FALLOFF) {
            curPupInfo->isShineThiefFallenOff = true;
            curPupInfo->isShineThiefHolder = false;

            if (mMode) {
                mMode->setShinePos(shinePak->shinePos);

                ShineThiefIcon* layout = mMode->getLayout();
                if (layout) {
                    layout->showShineRespawned();
                }
            }
        }

        // Update shine position when puppet is holding it
        if (puppetIsNowHolder && shinePak->updateType != ShineThiefUpdateType::FALLOFF) {
            sead::Vector3f offset{0, 100, 0};
            if (mMode) {
                mMode->setShinePos(curPupInfo->playerPos + offset);
            }
        }
    }
}

/**
 * @brief
 *
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
 * @brief
 *
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
}

/**
 * @brief
 *
 * @param shineId
 * @return true
 * @return false
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
 * @brief
 *
 * @param id
 * @return int
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
 * @brief
 *
 * @param holder
 */
void Client::setStageInfo(GameDataHolderAccessor holder) {
    if (sInstance) {
        sInstance->mStageName = GameDataFunction::getCurrentStageName(holder);
        sInstance->mScenario = holder.mData->getGameDataFile()->getScenarioNo();  // holder.mData->mGameDataFile->getMainScenarioNoCurrent();

        sInstance->mPuppetHolder->setStageInfo(sInstance->mStageName.cstr(), sInstance->mScenario);
    }
}

/**
 * @brief
 *
 * @param puppet
 * @return true
 * @return false
 */
bool Client::tryAddPuppet(PuppetActor* puppet) {
    if (sInstance) {
        return sInstance->mPuppetHolder->tryRegisterPuppet(puppet);
    } else {
        return false;
    }
}

/**
 * @brief
 *
 * @param puppet
 * @return true
 * @return false
 */
bool Client::tryAddDebugPuppet(PuppetActor* puppet) {
    if (sInstance) {
        return sInstance->mPuppetHolder->tryRegisterDebugPuppet(puppet);
    } else {
        return false;
    }
}

/**
 * @brief
 *
 * @param idx
 * @return PuppetActor*
 */
PuppetActor* Client::getPuppet(int idx) {
    if (sInstance) {
        return sInstance->mPuppetHolder->getPuppetActor(idx);
    } else {
        return nullptr;
    }
}

/**
 * @brief
 *
 * @return PuppetInfo*
 */
PuppetInfo* Client::getLatestInfo() {
    if (sInstance) {
        return Client::getPuppetInfo(sInstance->mPuppetHolder->getSize() - 1);
    } else {
        return nullptr;
    }
}

/**
 * @brief
 *
 * @param idx
 * @return PuppetInfo*
 */
PuppetInfo* Client::getPuppetInfo(int idx) {
    if (sInstance) {
        // unsafe get
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

/**
 * @brief
 *
 */
void Client::resetCollectedShines() {
    collectedShineCount = 0;
    curCollectedShines.fill(-1);
}

/**
 * @brief
 *
 * @param shineId
 */
void Client::removeShine(int shineId) {
    for (size_t i = 0; i < curCollectedShines.size(); i++) {
        if (curCollectedShines[i] == shineId) {
            curCollectedShines[i] = -1;
            collectedShineCount--;
        }
    }
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool Client::isNeedUpdateShines() {
    return sInstance ? sInstance->collectedShineCount > 0 : false;
}

/**
 * @brief
 *
 */
void Client::updateShines() {
    if (!sInstance) {
        Logger::log("Client Null!\n");
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
    sInstance->mCurStageScene->stageSceneLayout->updateCounterParts();  // updates shine chip layout to (maybe) prevent softlocks
}

/**
 * @brief
 *
 */
void Client::update() {
    if (sInstance) {
        sInstance->mPuppetHolder->update();

        if (isNeedUpdateShines()) {
            updateShines();
        }

        GameModeManager::instance()->update();
    }
}

/**
 * @brief
 *
 */
void Client::clearArrays() {
    if (sInstance) {
        sInstance->mPuppetHolder->clearPuppets();
        sInstance->mShineArray.clear();
        sInstance->mCoinCollectArray.clear();
        sInstance->mCoinCollect2DArray.clear();
    }
}

/**
 * @brief
 *
 */
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

/**
 * @brief
 *
 */
void Client::updateHealthCoins(HealthCoins* packet) {
    if (!sInstance) {
        return;
    }

    sInstance->mHealth = packet->health;
    sInstance->mCoins = packet->coins;
    sInstance->isKids = packet->isKids;
    sInstance->mNeedsUpdateHealthCoins = true;
}

/**
 * @brief
 *
 */
void Client::updateCoinCollects(CoinCollectCollect* packet) {
    al::PlacementId placeID(packet->placeID, nullptr, nullptr);
    GameDataFile* gdf = GameDataHolderAccessor(sInstance->mCurStageScene)->getGameDataFile();
    gdf->customAddCoinCollect(&placeID, packet->worldID, packet->stage);
    if (gdf->isGotCoinCollect(&placeID)) {
        for (int i = 0; i < sInstance->mCoinCollectArray.size(); i++) {
            if (sInstance->mCoinCollectArray[i]->mPlacementId->isEqual(placeID)) {
                sInstance->mCoinCollectArray[i]->makeActorDead();
                return;
            }
        }
        for (int i = 0; i < sInstance->mCoinCollect2DArray.size(); i++) {
            al::StringTmp<128> placeIDString;
            sInstance->mCoinCollect2DArray[i]->mPlacementId->makeString(&placeIDString);
            if (placeIDString.isEqual(packet->placeID)) {
                sInstance->mCoinCollect2DArray[i]->makeActorDead();
                return;
            }
        }
    }
}

/**
 * @brief
 *
 */
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

/**
 * @brief
 *
 * @return PuppetInfo*
 */
PuppetInfo* Client::getDebugPuppetInfo() {
    if (sInstance) {
        return &sInstance->mDebugPuppetInfo;
    } else {
        return nullptr;
    }
}

/**
 * @brief
 *
 * @return PuppetActor*
 */
PuppetActor* Client::getDebugPuppet() {
    if (sInstance) {
        return sInstance->mPuppetHolder->getDebugPuppet();
    } else {
        return nullptr;
    }
}

/**
 * @brief
 *
 * @return Keyboard*
 */
Keyboard* Client::getKeyboard() {
    if (sInstance) {
        return sInstance->mKeyboard;
    }
    return nullptr;
}

/**
 * @brief
 *
 * @return const char*
 */
const char* Client::getCurrentIP() {
    if (sInstance) {
        return sInstance->mServerIP.cstr();
    }
    return nullptr;
}

/**
 * @brief
 *
 * @return const int
 */
const int Client::getCurrentPort() {
    if (sInstance) {
        return sInstance->mServerPort;
    }
    return -1;
}

/**
 * @brief
 *
 * @return const bool
 */
const bool Client::hasServerChanged() {
    if (!sInstance) {
        return false;
    }
    return (getCurrentPort() != sInstance->mSocket->getPort() || strcmp(getCurrentIP(), sInstance->mSocket->getIP()) != 0);
}

/**
 * @brief sets server IP to supplied string, used specifically for loading IP from the save file.
 *
 * @param ip
 */
void Client::setLastUsedIP(const char* ip) {
    if (sInstance) {
        sInstance->mServerIP = ip;
    }
}

/**
 * @brief sets server port to supplied string, used specifically for loading port from the save
 * file.
 *
 * @param port
 */
void Client::setLastUsedPort(const int port) {
    if (sInstance) {
        sInstance->mServerPort = port;
    }
}

/**
 * @brief creates new scene info and copies supplied info to the new info, as well as stores a const
 * ptr to the current stage scene.
 *
 * @param initInfo
 * @param stageScene
 */
void Client::setSceneInfo(const al::ActorInitInfo& initInfo, const StageScene* stageScene) {
    if (!sInstance) {
        Logger::log("Client Null!\n");
        return;
    }

    sInstance->mSceneInfo = new al::ActorSceneInfo();

    memcpy(sInstance->mSceneInfo, &initInfo.actorSceneInfo, sizeof(al::ActorSceneInfo));

    sInstance->mCurStageScene = stageScene;
}

/**
 * @brief stores shine pointer supplied into a ptr array if space is available, and shine is not
 * collected.
 *
 * @param shine
 * @return true if shine was able to be successfully stored
 * @return false if shine is already collected, or ptr array is full
 */
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

/**
 * @brief stores CoinCollect pointer supplied into a ptr array if space is available.
 *
 * @param coin
 */
void Client::tryRegisterCoinCollect(CoinCollect* coin) {
    if (sInstance) {
        if (!sInstance->mCoinCollectArray.isFull()) {
            sInstance->mCoinCollectArray.pushBack(coin);
        }
    }
}

/**
 * @brief stores CoinCollect2D pointer supplied into a ptr array if space is available.
 *
 * @param coin
 */
void Client::tryRegisterCoinCollect2D(CoinCollect2D* coin) {
    if (sInstance) {
        if (!sInstance->mCoinCollect2DArray.isFull()) {
            sInstance->mCoinCollect2DArray.pushBack(coin);
        }
    }
}

/**
 * @brief finds the actor pointer stored in the shine ptr array based off shine ID
 *
 * @param shineID Unique ID used for shine actor
 * @return Shine* if shine ptr array contains actor with supplied shine ID.
 */
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

    al::hidePane(sInstance->mUIMessage, "Page01");  // hide A button prompt

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