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
#include "Library/Memory/HeapUtil.h"
#include "Library/Yaml/ByamlIter.h"
#include "Library/Yaml/ByamlUtil.h"
#include "logger.hpp"
#include "main.hpp"
#include "packets/InitPacket.h"
#include "packets/MoonRockHit.h"
#include "packets/Packet.h"
#include "prim/seadSafeString.h"
#include "puppets/PuppetInfo.h"
#include "Scene/StageScene.h"
#include "Sequence/HakoniwaSequence.h"
#include "server/SocketClient.hpp"
#include "server/LegacyProtocol.hpp"
#include "System/GameDataHolder.h"
#include "System/GameDataHolderAccessor.h"
#include "System/GameDataHolderWriter.h"
#include "System/UniqObjInfo.h"
#include "types.h"
#include "Util/AchievementUtil.h"

SEAD_SINGLETON_DISPOSER_IMPL(Client)

/**
 * @brief Construct a new Client::Client object
 */
Client::Client() {
    sead::ScopedCurrentHeapSetter setter(gHeap);

    mReadThread = new al::AsyncFunctorThread("ClientReadThread", al::FunctorV0M(this, &Client::readFunc), 0,
                                             16_KB, sead::CoreId::cMain);

    mKeyboard = new Keyboard(nn::swkbd::GetRequiredStringBufferSize());

    mSocket = new SocketClient();

    mPuppetHolder = new PuppetHolder(MAXPUPINDEX - 1);

    for (size_t i = 0; i < MAXPUPINDEX; i++) {
        mPuppetInfoArr[i] = new PuppetInfo();

        sprintf(mPuppetInfoArr[i]->puppetName, "Puppet%zu", i);
    }

    mConnectCount = 0;

    curCollectedShines.fill(-1);
    mPendingMoonRocks.fill(false);

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
    sead::ScopedCurrentHeapSetter setter(al::getSequenceHeap());

    mUIMessage = new al::WindowConfirmWait("ServerWaitConnect", "WindowConfirmWait", initInfo);
    mConnectStatus = new al::SimpleLayoutAppearWaitEnd("", "SaveMessage", initInfo, 0, false);
    ConnectionStatus::sInstance = new ConnectionStatus("Status", initInfo);
    SpeedrunIcon::sInstance = new SpeedrunIcon("SpeedrunIcon", initInfo);

    sead::ScopedCurrentHeapSetter setter2(gHeap);

    if (PlayerEventLog::instance())
        PlayerEventLog::deleteInstance();
    PlayerEventLog::createInstance(gHeap);

    if (mUIMessage) {
        mUIMessage->setTxtMessage(u"Connecting to Server.");
        mUIMessage->setTxtMessageConfirm(u"Failed to Connect!");
    }
    if (mConnectStatus) {
        al::setPaneString(mConnectStatus, "TxtSave", u"Connecting to Server.", 0);
        al::setPaneString(mConnectStatus, "TxtSaveSh", u"Connecting to Server.", 0);
    }

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
    if (sInstance->mIsAllowReconnect)
        sInstance->mSocket->setSocketClientState(SocketClient::SocketClientState::RESET);
}

/**
 * @brief starts a connection using client's TCP socket class, pulling up the software keyboard for
 * user inputted IP if save file does not have one saved.
 *
 * @return true if successful connection to server
 * @return false if connection was unable to establish
 */
bool Client::startConnection() {
    sead::ScopedCurrentHeapSetter setter(gHeap);
    // This function runs on the connection worker.  It must not open Odyssey
    // UI, read input, or mutate save data.  The configuration menu owns those
    // operations on the game thread; use a safe local default for first run.
    if (mServerIP.isEmpty()) {
        mServerIP = "127.0.0.1";
    }

    if (!mServerPort) {
        mServerPort = 1027;
    }

    mSocket->init(mServerIP.cstr(), mServerPort);

    while (mSocket->getSocketClientState() == SocketClient::INIT) {
        hk::diag::logLine("log state: %s", mSocket->getStateChar());
        nn::os::YieldThread();
        nn::os::SleepThread(nn::TimeSpan::FromNanoSeconds(100_ms));
    }

    // The init frame is decoded and applied by Client::update() on the game
    // thread.  Do not touch puppet/UI/game state from this connection thread.
    mIsConnectionActive = mSocket->getLogState() == SockState::CONNECTED;
    mLegacyProfileActive = false;
    mIsAllowReconnect = false;
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
    if (!sInstance || !sInstance->mUIMessage) {
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
    if (!sInstance || !sInstance->mUIMessage) {
        return;
    }

    sInstance->mUIMessage->tryEnd();
}

/**
 * @brief main thread function for read thread, responsible for processing packets from server
 *
 */
void Client::readFunc() {
    sead::ScopedCurrentHeapSetter setter(gHeap);

    hk::diag::logLine("Starting Client read thread");

    /*if (isFirstRun) {
        // wait for some stuff to init
        nn::os::YieldThread();
        nn::os::SleepThread(nn::TimeSpan::FromSeconds(2));
    }*/

    if (!startConnection()) {
        hk::diag::logLine("Failed to Connect to Server.");
        return;
    }

    isFirstRun = false;
    hk::diag::logLine("Client connection thread ending; game thread owns frame application.");
}

namespace {
bool isSafeStageToken(const char* value) {
    if (!value || value[0] == '\0')
        return false;
    for (const char* p = value; *p; ++p) {
        const bool allowed = (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
                             (*p >= '0' && *p <= '9') || *p == '_' || *p == '-';
        if (!allowed)
            return false;
    }
    return true;
}

bool isAllowedStageDestination(const char* stage) {
    static constexpr const char* kStages[] = {
        "CapWorldHomeStage",      "WaterfallWorldHomeStage", "SandWorldHomeStage",
        "ForestWorldHomeStage",   "LakeWorldHomeStage",      "CloudWorldHomeStage",
        "ClashWorldHomeStage",    "CityWorldHomeStage",      "SeaWorldHomeStage",
        "SnowWorldHomeStage",     "LavaWorldHomeStage",      "BossRaidWorldHomeStage",
        "SkyWorldHomeStage",      "MoonWorldHomeStage",      "PeachWorldHomeStage",
        "Special1WorldHomeStage", "Special2WorldHomeStage",
    };
    for (const char* allowed : kStages) {
        if (std::strcmp(stage, allowed) == 0)
            return true;
    }
    return false;
}

bool isAllowedStageEntrance(const char* entrance) {
    static constexpr const char* kEntrances[] = {"Start", "StartDemo", "Warp", "MoonRock"};
    for (const char* allowed : kEntrances) {
        if (std::strcmp(entrance, allowed) == 0)
            return true;
    }
    return false;
}

bool isValidAnimation(PlayerAnims::Type type) {
    const s16 value = static_cast<s16>(type);
    return value == static_cast<s16>(PlayerAnims::Type::Unknown) ||
           (value >= 0 && value < static_cast<s16>(PlayerAnims::Type::End));
}

nn::account::Uid makeUid(const LegacyProtocol::Header& header) {
    nn::account::Uid id{};
    std::memcpy(id.data, header.userId, sizeof(header.userId));
    return id;
}
}  // namespace

bool Client::queueLegacyFrame(s16 type, const u8* payload, s16 payloadSize) {
    if (!mLegacyProfileActive || !mSocket || !payload || payloadSize < 0 ||
        payloadSize > LegacyProtocol::MaxPayloadSize)
        return false;

    u8 frame[LegacyProtocol::MaxFrameSize] = {};
    LegacyProtocol::encodeHeader(frame, reinterpret_cast<const LegacyProtocol::Byte*>(mUserID.data), type,
                                 payloadSize);
    if (payloadSize > 0)
        std::memcpy(frame + LegacyProtocol::HeaderSize, payload, payloadSize);
    return mSocket->queueFrame(frame, LegacyProtocol::HeaderSize + payloadSize);
}

void Client::processIncomingFrames() {
    if (!mSocket)
        return;

    // Keep frame work bounded per game tick.  Player/cap packets are snapshots,
    // so processing the next update is preferable to blocking scene work.
    for (s32 i = 0; i < 32; i++) {
        u8* frame = mSocket->tryGetFrame();
        if (!frame)
            return;
        handleLegacyFrame(frame);
        delete[] frame;
    }
}

void Client::handleLegacyFrame(const u8* frame) {
    LegacyProtocol::Header header{};
    if (!LegacyProtocol::decodeHeader(frame, LegacyProtocol::HeaderSize, &header))
        return;

    // Known layouts must match exactly.  The three server extension packets
    // without public layouts are still consumed by SocketClient and ignored.
    if (!LegacyProtocol::hasExpectedPayloadSize(header)) {
        hk::diag::logLine("Malformed legacy frame type %d size %d; reconnecting.", header.type,
                          header.payloadSize);
        mSocket->setSocketClientState(SocketClient::SocketClientState::RESET);
        return;
    }

    const u8* payload = frame + LegacyProtocol::HeaderSize;
    const nn::account::Uid userId = makeUid(header);

    if (header.type == LegacyProtocol::Init) {
        if (header.payloadSize != 34)
            return;

        u16 serverMaxPlayers = 0;
        char version[33] = {};
        if (!LegacyProtocol::read(payload, header.payloadSize, 0, &serverMaxPlayers) ||
            !LegacyProtocol::copyFixedString(version, sizeof(version), payload, header.payloadSize, 2, 32) ||
            std::strcmp(version, LegacyProtocol::ServerVersion) != 0) {
            hk::diag::logLine("Unsupported server layout/version '%s'.", version);
            setServerVersion("Unsupported server");
            showUIMessage(u"Unsupported server. Expected SMOO+ 0.5 pre.");
            mLegacyProfileActive = false;
            mIsConnectionActive = false;
            mIsAllowReconnect = false;
            mSocket->setSocketClientState(SocketClient::SocketClientState::RESET);
            return;
        }

        const s32 remoteCapacity = static_cast<s32>(serverMaxPlayers) - 1;
        maxPuppets = remoteCapacity < 0 ? 0 : (remoteCapacity > MAXPUPINDEX - 1 ? MAXPUPINDEX - 1 : remoteCapacity);
        setServerVersion(version);
        mLegacyProfileActive = true;
        mIsConnectionActive = true;
        mIsAllowReconnect = true;
        hk::diag::logLine("Legacy SMOO+ 0.5 pre profile active (%d remote-player slots).", maxPuppets);
        return;
    }

    if (!mLegacyProfileActive)
        return;

    switch (header.type) {
    case LegacyProtocol::Player: {
        PlayerInf packet;
        packet.mUserID = userId;
        if (!LegacyProtocol::read(payload, 56, 0, &packet.playerPos) ||
            !LegacyProtocol::read(payload, 56, 12, &packet.playerRot) ||
            !LegacyProtocol::read(payload, 56, 28, &packet.animBlendWeights) ||
            !LegacyProtocol::read(payload, 56, 52, &packet.actName) ||
            !LegacyProtocol::read(payload, 56, 54, &packet.subActName) ||
            !LegacyProtocol::isFiniteVec3(packet.playerPos.x, packet.playerPos.y, packet.playerPos.z) ||
            !LegacyProtocol::isNormalizedQuat(packet.playerRot.x, packet.playerRot.y, packet.playerRot.z,
                                               packet.playerRot.w) ||
            !isValidAnimation(packet.actName) || !isValidAnimation(packet.subActName))
            return;
        for (float weight : packet.animBlendWeights) {
            if (!LegacyProtocol::isFinite(weight) || weight < 0.0f || weight > 1.0f)
                return;
        }
        updatePlayerInfo(&packet);
        break;
    }
    case LegacyProtocol::Cap: {
        sead::Vector3f pos{};
        sead::Quatf rotation{};
        bool1 visible = false;
        char animation[PACKBUFSIZE] = {};
        if (!LegacyProtocol::read(payload, 80, 0, &pos) || !LegacyProtocol::read(payload, 80, 12, &rotation) ||
            !LegacyProtocol::read(payload, 80, 28, &visible) ||
            !LegacyProtocol::copyFixedString(animation, sizeof(animation), payload, 80, 32, 48) ||
            !LegacyProtocol::isFiniteVec3(pos.x, pos.y, pos.z) ||
            !LegacyProtocol::isNormalizedQuat(rotation.x, rotation.y, rotation.z, rotation.w) || visible > 1)
            return;
        PuppetInfo* info = findPuppetInfo(userId, false);
        if (!info)
            return;
        info->capPos = pos;
        // The server sends one quaternion only.  It is the cap rotation; the
        // newer client-only second quaternion is never read from this frame.
        info->capRot = rotation;
        info->capQuat = sead::Quatf::unit;
        info->isCapThrow = visible;
        std::strncpy(info->capAnim, animation, sizeof(info->capAnim) - 1);
        info->capAnim[sizeof(info->capAnim) - 1] = '\0';
        break;
    }
    case LegacyProtocol::Game: {
        GameInf packet;
        packet.mUserID = userId;
        if (!LegacyProtocol::read(payload, 66, 0, &packet.is2D) ||
            !LegacyProtocol::read(payload, 66, 1, &packet.scenarioNo) ||
            !LegacyProtocol::copyFixedString(packet.stageName, sizeof(packet.stageName), payload, 66, 2, 64) ||
            !isSafeStageToken(packet.stageName) || packet.is2D > 1 || packet.scenarioNo > 99)
            return;
        // SMOO+ 0.5 pre has no gameMode field.
        packet.gameMode = -1;
        updateGameInfo(&packet);
        break;
    }
    case LegacyProtocol::Connect: {
        PlayerConnect packet;
        packet.mUserID = userId;
        s32 connectionType = 0;
        if (!LegacyProtocol::read(payload, 38, 0, &connectionType) ||
            !LegacyProtocol::read(payload, 38, 4, &packet.maxPlayerCount) ||
            !LegacyProtocol::copyFixedString(packet.clientName, sizeof(packet.clientName), payload, 38, 6, 32) ||
            connectionType < 0 || connectionType > 1)
            return;
        updatePlayerConnect(&packet);
        resendCachedState();
        break;
    }
    case LegacyProtocol::Disconnect: {
        PlayerDC packet;
        packet.mUserID = userId;
        disconnectPlayer(&packet);
        break;
    }
    case LegacyProtocol::Costume:
    case LegacyProtocol::ChangeCostume: {
        CostumeInf packet;
        packet.mUserID = userId;
        if (!LegacyProtocol::copyFixedString(packet.bodyModel, sizeof(packet.bodyModel), payload, 64, 0, 32) ||
            !LegacyProtocol::copyFixedString(packet.capModel, sizeof(packet.capModel), payload, 64, 32, 32))
            return;
        updateCostumeInfo(&packet);
        break;
    }
    case LegacyProtocol::Shine: {
        ShineCollect packet;
        packet.mUserID = userId;
        if (!LegacyProtocol::read(payload, 4, 0, &packet.shineId) || packet.shineId < 0 || packet.shineId > 99999)
            return;
        // The legacy server sends only the integer ID, not the trailing flag.
        packet.isGrand = false;
        updateShineInfo(&packet);
        break;
    }
    case LegacyProtocol::Capture: {
        CaptureInf packet;
        packet.mUserID = userId;
        if (!LegacyProtocol::copyFixedString(packet.hackName, sizeof(packet.hackName), payload, 32, 0, 32))
            return;
        updateCaptureInfo(&packet);
        break;
    }
    case LegacyProtocol::ChangeStage: {
        ChangeStagePacket packet;
        packet.mUserID = userId;
        if (!LegacyProtocol::copyFixedString(packet.changeStage, sizeof(packet.changeStage), payload, 68, 0, 48) ||
            !LegacyProtocol::copyFixedString(packet.changeID, sizeof(packet.changeID), payload, 68, 48, 16) ||
            !LegacyProtocol::read(payload, 68, 64, &packet.scenarioNo) ||
            !LegacyProtocol::read(payload, 68, 65, &packet.subScenarioType) ||
            !isSafeStageToken(packet.changeStage) || !isSafeStageToken(packet.changeID) ||
            !isAllowedStageDestination(packet.changeStage) || !isAllowedStageEntrance(packet.changeID) ||
            packet.scenarioNo < 0 || packet.scenarioNo > 99 || packet.subScenarioType > 15)
            return;
        sendToStage(&packet);
        break;
    }
    // These server packet IDs intentionally do not share the old client enum.
    // Their complete payload was already consumed by SocketClient; unsupported
    // chat/UDP/hole-punch/extra/health/mod packets are ignored safely.
    case LegacyProtocol::Tag:
    case LegacyProtocol::Command:
    case LegacyProtocol::Chat:
    case LegacyProtocol::UdpInit:
    case LegacyProtocol::HolePunch:
    case LegacyProtocol::Extra:
    case LegacyProtocol::HealthCoins:
    case LegacyProtocol::Mods:
    default: break;
    }
}

void Client::resendCachedState() {
    // A new peer needs a reliable snapshot.  Re-encode cached state through
    // the legacy codec; never hand an in-memory Packet to the socket layer.
    if (lastGameInfPacket.stageName[0] != '\0') {
        u8 payload[66] = {};
        LegacyProtocol::write(payload, sizeof(payload), 0, lastGameInfPacket.is2D);
        LegacyProtocol::write(payload, sizeof(payload), 1, lastGameInfPacket.scenarioNo);
        std::memcpy(payload + 2, lastGameInfPacket.stageName, sizeof(lastGameInfPacket.stageName));
        queueLegacyFrame(LegacyProtocol::Game, payload, sizeof(payload));
    }

    if (lastPlayerInfPacket.mUserID == mUserID) {
        u8 payload[56] = {};
        LegacyProtocol::write(payload, sizeof(payload), 0, lastPlayerInfPacket.playerPos);
        LegacyProtocol::write(payload, sizeof(payload), 12, lastPlayerInfPacket.playerRot);
        LegacyProtocol::write(payload, sizeof(payload), 28, lastPlayerInfPacket.animBlendWeights);
        LegacyProtocol::write(payload, sizeof(payload), 52, lastPlayerInfPacket.actName);
        LegacyProtocol::write(payload, sizeof(payload), 54, lastPlayerInfPacket.subActName);
        queueLegacyFrame(LegacyProtocol::Player, payload, sizeof(payload));
    }

    if (lastCostumeInfPacket.bodyModel[0] != '\0') {
        u8 payload[64] = {};
        std::memcpy(payload, lastCostumeInfPacket.bodyModel, sizeof(lastCostumeInfPacket.bodyModel));
        std::memcpy(payload + 32, lastCostumeInfPacket.capModel, sizeof(lastCostumeInfPacket.capModel));
        queueLegacyFrame(LegacyProtocol::Costume, payload, sizeof(payload));
    }

    u8 capture[32] = {};
    std::memcpy(capture, lastCaptureInfPacket.hackName, sizeof(lastCaptureInfPacket.hackName));
    queueLegacyFrame(LegacyProtocol::Capture, capture, sizeof(capture));
}

void Client::sendPlayerInfPacket(const PlayerActorBase* playerBase, bool isYukimaru) {
    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    if (!playerBase) {
        hk::diag::logLine("Error: Null Player Reference");
        return;
    }

    PlayerInf packet;
    packet.mUserID = sInstance->mUserID;

    packet.playerPos = al::getTrans(playerBase);

    al::calcQuat(&packet.playerRot, playerBase);

    if (!isYukimaru) {
        PlayerActorHakoniwa* player = (PlayerActorHakoniwa*)playerBase;

        for (size_t i = 0; i < 6; i++) {
            packet.animBlendWeights[i] = player->mAnimator->getBlendWeight(i);
        }

        const char* hackName = player->mHackKeeper->getCurrentHackName();

        if (hackName != nullptr) {
            sInstance->isClientCaptured = true;

            const char* actName = al::getActionName(player->mHackKeeper->mHackActor);

            if (actName) {
                packet.actName = PlayerAnims::FindType(actName);
                packet.subActName = PlayerAnims::Type::Unknown;
            } else {
                packet.actName = PlayerAnims::Type::Unknown;
                packet.subActName = PlayerAnims::Type::Unknown;
            }
        } else {
            packet.actName = PlayerAnims::FindType(player->mAnimator->mAnimFrameCtrl->getActionName());
            packet.subActName = PlayerAnims::FindType(player->mAnimator->mCurSubAnim.cstr());

            sInstance->isClientCaptured = false;
        }

    } else {
        // TODO: implement YukimaruRacePlayer syncing

        for (size_t i = 0; i < 6; i++) {
            packet.animBlendWeights[i] = 0;
        }

        sInstance->isClientCaptured = false;

        packet.actName = PlayerAnims::Type::Unknown;
        packet.subActName = PlayerAnims::Type::Unknown;
    }

    static u32 idleSnapshotTicks = 0;
    if (sInstance->lastPlayerInfPacket == packet) {
        // State snapshots run at 20 Hz while changing, but an idle player only
        // refreshes once per second so TCP never accumulates stale transforms.
        if (++idleSnapshotTicks < 20)
            return;
    }

    u8 payload[56] = {};
    LegacyProtocol::write(payload, sizeof(payload), 0, packet.playerPos);
    LegacyProtocol::write(payload, sizeof(payload), 12, packet.playerRot);
    LegacyProtocol::write(payload, sizeof(payload), 28, packet.animBlendWeights);
    LegacyProtocol::write(payload, sizeof(payload), 52, packet.actName);
    LegacyProtocol::write(payload, sizeof(payload), 54, packet.subActName);
    if (sInstance->queueLegacyFrame(LegacyProtocol::Player, payload, sizeof(payload))) {
        sInstance->lastPlayerInfPacket = packet;
        idleSnapshotTicks = 0;
    }
}

/**
 * @brief sends info related to player's cap actor to server
 *
 * @param hackCap pointer to cap actor, used to get translation, animation, and state info
 */
void Client::sendHackCapInfPacket(const HackCap* hackCap) {
    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    bool isFlying = hackCap->isFlying();

    if (isFlying) {
        HackCapInf packet;
        packet.mUserID = sInstance->mUserID;
        packet.capPos = al::getTrans(hackCap);

        packet.isCapVisible = isFlying;

        packet.capQuat.x = hackCap->mJointKeeper->mJointRot.x;
        packet.capQuat.y = hackCap->mJointKeeper->mJointRot.y;
        packet.capQuat.z = hackCap->mJointKeeper->mJointRot.z;
        packet.capQuat.w = hackCap->mJointKeeper->mSkew;
        packet.capRotQuat = al::getQuat(hackCap);

        strncpy(packet.capAnim, al::getActionName(hackCap), sizeof(HackCapInf::capAnim) - 1);
        packet.capAnim[sizeof(HackCapInf::capAnim) - 1] = '\0';

        u8 payload[80] = {};
        LegacyProtocol::write(payload, sizeof(payload), 0, packet.capPos);
        // SMOO+ 0.5 pre has one legacy cap quaternion at offset 12.
        LegacyProtocol::write(payload, sizeof(payload), 12, packet.capQuat);
        LegacyProtocol::write(payload, sizeof(payload), 28, packet.isCapVisible);
        std::memcpy(payload + 32, packet.capAnim, sizeof(packet.capAnim));
        sInstance->queueLegacyFrame(LegacyProtocol::Cap, payload, sizeof(payload));

        sInstance->isSentHackInf = true;

    } else if (sInstance->isSentHackInf) {
        u8 payload[80] = {};
        const sead::Vector3f pos = sead::Vector3f::zero;
        const sead::Quatf rot = sead::Quatf::unit;
        const bool1 visible = false;
        LegacyProtocol::write(payload, sizeof(payload), 0, pos);
        LegacyProtocol::write(payload, sizeof(payload), 12, rot);
        LegacyProtocol::write(payload, sizeof(payload), 28, visible);
        sInstance->queueLegacyFrame(LegacyProtocol::Cap, payload, sizeof(payload));
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
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    GameInf packet;
    packet.mUserID = sInstance->mUserID;

    if (player) {
        packet.is2D = player->mDimensionKeeper->mIs2D;
    } else {
        packet.is2D = false;
    }

    packet.scenarioNo = holder.mData->getGameDataFile()->getScenarioNo();

    strncpy(packet.stageName, GameDataFunction::getCurrentStageName(holder), sizeof(GameInf::stageName) - 1);
    packet.stageName[sizeof(GameInf::stageName) - 1] = '\0';

    packet.gameMode = -1;

    if (packet == sInstance->lastGameInfPacket)
        return;
    u8 payload[66] = {};
    LegacyProtocol::write(payload, sizeof(payload), 0, packet.is2D);
    LegacyProtocol::write(payload, sizeof(payload), 1, packet.scenarioNo);
    std::memcpy(payload + 2, packet.stageName, sizeof(packet.stageName));
    if (sInstance->queueLegacyFrame(LegacyProtocol::Game, payload, sizeof(payload)))
        sInstance->lastGameInfPacket = packet;
}

/**
 * @brief Sends only stage info to the server.
 * @param holder
 */
void Client::sendGameInfPacket(GameDataHolderAccessor holder) {
    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    GameInf packet;
    packet.mUserID = sInstance->mUserID;
    packet.is2D = false;
    packet.scenarioNo = holder.mData->getGameDataFile()->getScenarioNo();
    strncpy(packet.stageName, GameDataFunction::getCurrentStageName(holder), sizeof(GameInf::stageName) - 1);
    packet.stageName[sizeof(GameInf::stageName) - 1] = '\0';
    packet.gameMode = -1;

    u8 payload[66] = {};
    LegacyProtocol::write(payload, sizeof(payload), 0, packet.is2D);
    LegacyProtocol::write(payload, sizeof(payload), 1, packet.scenarioNo);
    std::memcpy(payload + 2, packet.stageName, sizeof(packet.stageName));
    if (sInstance->queueLegacyFrame(LegacyProtocol::Game, payload, sizeof(payload)))
        sInstance->lastGameInfPacket = packet;
}

/**
 * @brief Sends costume info packet.
 * @param body
 * @param cap
 */
void Client::sendCostumeInfPacket(const char* body, const char* cap) {
    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    CostumeInf packet;
    packet.mUserID = sInstance->mUserID;
    if (body)
        std::strncpy(packet.bodyModel, body, sizeof(packet.bodyModel) - 1);
    if (cap)
        std::strncpy(packet.capModel, cap, sizeof(packet.capModel) - 1);
    u8 payload[64] = {};
    std::memcpy(payload, packet.bodyModel, sizeof(packet.bodyModel));
    std::memcpy(payload + 32, packet.capModel, sizeof(packet.capModel));
    if (sInstance->queueLegacyFrame(LegacyProtocol::Costume, payload, sizeof(payload)))
        sInstance->lastCostumeInfPacket = packet;
}

/**
 * @brief Sends capture info packet.
 * @param player
 */
void Client::sendCaptureInfPacket(const PlayerActorHakoniwa* player) {
    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    if (sInstance->isClientCaptured && !sInstance->isSentCaptureInf) {
        CaptureInf packet;
        packet.mUserID = sInstance->mUserID;
        strncpy(packet.hackName, tryConvertName(player->mHackKeeper->getCurrentHackName()),
                sizeof(packet.hackName) - 1);
        u8 payload[32] = {};
        std::memcpy(payload, packet.hackName, sizeof(packet.hackName));
        if (sInstance->queueLegacyFrame(LegacyProtocol::Capture, payload, sizeof(payload))) {
            sInstance->lastCaptureInfPacket = packet;
            sInstance->isSentCaptureInf = true;
        }
    } else if (!sInstance->isClientCaptured && sInstance->isSentCaptureInf) {
        CaptureInf packet;
        packet.mUserID = sInstance->mUserID;
        u8 payload[32] = {};
        if (sInstance->queueLegacyFrame(LegacyProtocol::Capture, payload, sizeof(payload))) {
            sInstance->lastCaptureInfPacket = packet;
            sInstance->isSentCaptureInf = false;
        }
    }
}

/**
 * @brief Sends shine collect packet.
 * @param shineID
 */
void Client::sendShineCollectPacket(int shineID) {
    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    if (sInstance->lastCollectedShine != shineID) {
        u8 payload[4] = {};
        LegacyProtocol::write(payload, sizeof(payload), 0, shineID);
        if (sInstance->queueLegacyFrame(LegacyProtocol::Shine, payload, sizeof(payload)))
            sInstance->lastCollectedShine = shineID;
    }
}

/**
 * @brief Sends coin collect packet.
 * @param placeID
 * @param worldID
 * @param stage
 */
void Client::sendCoinCollectCollectPacket(const char* placeID, int worldID, const char* stage) {
    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    (void)placeID;
    (void)worldID;
    (void)stage;
    static bool warned = false;
    if (!warned) {
        hk::diag::logLine("Regional-coin sync is disabled for SMOO+ 0.5 pre (wire ID collision).");
        warned = true;
    }
}

/**
 * @brief Sends checkpoint get packet.
 * @param objId
 */
void Client::sendCheckpointGetPacket(const char* objId) {
    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    (void)objId;
    static bool warned = false;
    if (!warned) {
        hk::diag::logLine("Checkpoint sync is disabled for SMOO+ 0.5 pre (wire ID collision).");
        warned = true;
    }
}

/**
 * @brief Sends game start packet.
 */
void Client::sendGameStartPacket() {
    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    static bool warned = false;
    if (!warned) {
        hk::diag::logLine("Game-start sync is disabled for SMOO+ 0.5 pre (wire ID collision).");
        warned = true;
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

    if (LegacyProtocol::isNormalizedQuat(packet->playerRot.x, packet->playerRot.y, packet->playerRot.z,
                                         packet->playerRot.w))
        curInfo->playerRot = packet->playerRot;

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
    curInfo->capPos = packet->capPos;
    curInfo->capRot = packet->capQuat;
    curInfo->capQuat = sead::Quatf::unit;
    curInfo->isCapThrow = packet->isCapVisible;
    strncpy(curInfo->capAnim, packet->capAnim, sizeof(PuppetInfo::capAnim) - 1);
    curInfo->capAnim[sizeof(PuppetInfo::capAnim) - 1] = '\0';
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
        if (!hintInfo) {
            hk::diag::logLine("Ignoring unknown shine ID %d.", packet->shineId);
            return;
        }
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
    }
}

/**
 * @brief Sends the player to a new stage as directed by a ChangeStagePacket.
 * @param packet
 */
void Client::sendToStage(ChangeStagePacket* packet) {
    if (!gIsSceneAlive || !mCurStageScene) {
        hk::diag::logLine("Ignoring stage change outside an active scene.");
        return;
    }

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

    if (mConnectCount > 0)
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

    const s32 slotCount = maxPuppets < 0 ? 0 : (maxPuppets > MAXPUPINDEX - 1 ? MAXPUPINDEX - 1 : maxPuppets);
    for (s32 i = 0; i < slotCount; i++) {
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
    if (sInstance && idx >= 0 && idx < sInstance->mPuppetHolder->getSize()) {
        return sInstance->mPuppetHolder->getPuppetActor(idx);
    }
    return nullptr;
}

PuppetInfo* Client::getLatestInfo() {
    if (sInstance) {
        return Client::getPuppetInfo(sInstance->mPuppetHolder->getSize() - 1);
    } else {
        return nullptr;
    }
}

PuppetInfo* Client::getPuppetInfo(int idx) {
    if (sInstance && idx >= 0 && idx < sInstance->maxPuppets && idx < MAXPUPINDEX) {
        PuppetInfo* curInfo = sInstance->mPuppetInfoArr[idx];

        if (!curInfo) {
            hk::diag::logLine("Attempting to Access Puppet Out of Bounds! Value: %d", idx);
            return nullptr;
        }

        return curInfo;
    }
    hk::diag::logLine("Attempting to Access Puppet Out of Bounds! Value: %d", idx);
    return nullptr;
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

    if (packet->worldId < 0 || packet->worldId >= SNumMoonRocks) {
        hk::diag::logLine("Ignoring invalid moon-rock world ID %d.", packet->worldId);
        return;
    }

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
    if (!sInstance) {
        hk::diag::logLine("Static Instance is Null!");
        return;
    }

    (void)worldId;
    static bool warned = false;
    if (!warned) {
        hk::diag::logLine("Moon-rock sync is disabled for SMOO+ 0.5 pre (wire ID collision).");
        warned = true;
    }
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
        // Network workers only enqueue immutable wire frames.  All engine,
        // puppet, UI/event-log and scene effects begin here on the game thread.
        sInstance->processIncomingFrames();

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
    if (!sInstance || !sInstance->mUIMessage)
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
    if (!sInstance || !sInstance->mUIMessage)
        return;

    sInstance->mUIMessage->appear();

    sInstance->mUIMessage->playLoop();
}

void Client::hideConnect() {
    if (!sInstance || !sInstance->mUIMessage)
        return;

    sInstance->mUIMessage->tryEnd();
}
