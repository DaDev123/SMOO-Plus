/**
 * @file server/Client.hpp
 * @author CraftyBoss (https://github.com/CraftyBoss)
 * @brief main class responsible for handing all client-server related communications, as well as
 * any gamemodes.
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

// ===== SYSTEM INCLUDES =====
#include <cstddef>
#include <stdlib.h>

// ===== AL/GAME ENGINE INCLUDES =====

#include "al/Library/Layout/LayoutInitInfo.h"
#include "al/Library/Layout/WindowConfirmWait.h"
#include "al/Library/LiveActor/ActorInitInfo.h"
#include "al/Library/LiveActor/ActorSceneInfo.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/Play/Layout/SimpleLayoutAppearWaitEnd.h"
#include "al/Library/Sequence/Sequence.h"
#include "al/Library/Thread/AsyncFunctorThread.h"

// ===== GAME INCLUDES =====
#include "game/Item/CoinCollect.h"
#include "game/Item/CoinCollect2D.h"
#include "game/Item/Shine.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Scene/StageScene.h"
#include "game/System/GameDataHolderAccessor.h"

// ===== NINTENDO SDK INCLUDES =====
#include "nn/account.h"

// ===== SEAD INCLUDES =====
#include "sead/basis/seadNew.h"
#include "sead/container/seadSafeArray.h"
#include "sead/heap/seadDisposer.h"
#include "sead/heap/seadExpHeap.h"
#include "sead/prim/seadSafeString.h"

#include "container/seadPtrArray.h"

// ===== PROJECT INCLUDES =====
#include "Keyboard.hpp"
#include "packets/Packet.h"
#include "puppets/PuppetHolder.hpp"
#include "puppets/PuppetInfo.h"
#include "server/SocketClient.hpp"
#include "syssocket/sockdefines.h"
#include "thread/seadMessageQueue.h"
#include "types.h"

// ===== CONSTANTS =====
#define MAXPUPINDEX 32

// ===== FORWARD DECLARATIONS =====
class HideAndSeekIcon;
struct HealthCoins;

// ===== STRUCTURES =====
struct UIDIndexNode {
    nn::account::Uid uid;
    int puppetIndex;
};

/**
 * @brief Holds coin collect data that arrived before the scene was ready.
 *        Drained each frame in Client::update() once mCurStageScene is valid.
 */
struct PendingCoinCollect {
    char placeID[64];
    int worldID;
    char stage[64];
};

// ===== MAIN CLASS =====
class Client {
    SEAD_SINGLETON_DISPOSER(Client)

public:
    // ===== CONSTRUCTOR/DESTRUCTOR =====
    Client();

    // ===== INITIALIZATION =====
    static Client* get();
    GameDataHolderAccessor& getHolder() { return mHolder; }
    void init(al::LayoutInitInfo const& initInfo, GameDataHolderAccessor holder);
    bool startThread();

    // ===== STATIC CONNECTION METHODS =====
    static void restartConnection();
    static bool isSocketActive() { return sInstance ? sInstance->mSocket->isConnected() : false; }
    static bool isFirstConnect() { return sInstance ? sInstance->mIsFirstConnect : false; }

    // ===== PLAYER CONNECTION METHODS =====
    bool isPlayerConnected(int index) { return mPuppetInfoArr[index]->isConnected; }
    static int getConnectCount() {
        if (sInstance)
            return sInstance->mConnectCount;
        return 0;
    }
    static int getMaxPlayerCount() { return sInstance ? sInstance->maxPuppets + 1 : 8; }

    // ===== SHINE MANAGEMENT =====
    static bool isNeedUpdateShines();
    bool isShineCollected(int shineId);
    int getCollectedShinesCount() { return curCollectedShines.size(); }
    int getShineID(int index) {
        if (index < curCollectedShines.size()) {
            return curCollectedShines[index];
        }
        return -1;
    }
    void resetCollectedShines();
    void removeShine(int shineId);
    static bool tryRegisterShine(Shine* shine);
    static Shine* findStageShine(int shineID);
    static void updateShines();

    // ==== COINCOLLECT MANAGEMENT ====
    static void tryRegisterCoinCollect(CoinCollect* coin);
    static void tryRegisterCoinCollect2D(CoinCollect2D* coin);

    // ===== PACKET SENDING METHODS =====
    static void sendHackCapInfPacket(const HackCap* hackCap);
    static void sendPlayerInfPacket(const PlayerActorBase* player, bool isYukimaru);
    static void sendGameInfPacket(const PlayerActorHakoniwa* player, GameDataHolderAccessor holder);
    static void sendGameInfPacket(GameDataHolderAccessor holder);
    static void sendCaptureInfPacket(const PlayerActorHakoniwa* player);
    static void sendCostumeInfPacket(const char* body, const char* cap);
    static void sendShineCollectPacket(int shineId);
    static void sendTagInfPacket();
    static void sendFreezeInfPacket();
    static void sendShineThiefInfPacket();
    static void sendPuppetPosInfoPacket();
    static void sendCoinCollectCollectPacket(const char* placeID, int worldID, const char* stage);
    static void sendScenarioSyncPacket(const char* changeStageName, s32 scenario);
    static void sendMessagePacket(const char* message, int messageType = 0);

    // ===== PUPPET MANAGEMENT =====
    static bool tryAddPuppet(PuppetActor* puppet);
    static bool tryAddDebugPuppet(PuppetActor* puppet);
    static PuppetActor* getPuppet(int idx);
    static PuppetInfo* getPuppetInfo(int idx);
    static PuppetInfo* getLatestInfo();
    static PuppetInfo* getDebugPuppetInfo();
    static PuppetActor* getDebugPuppet();
    static PuppetHolder* getPuppetHolder() {
        if (sInstance)
            return sInstance->mPuppetHolder;
        return nullptr;
    }

    // ===== CLIENT INFO GETTERS =====
    static const char* getClientName() { return sInstance ? sInstance->mUsername.cstr() : "Player"; }
    static nn::account::Uid getClientId() { return sInstance ? sInstance->mUserID : nn::account::Uid(); }
    static sead::FixedSafeString<0x20> getUsername() { return sInstance ? sInstance->mUsername : sead::FixedSafeString<0x20>::cEmptyString; }
    sead::FixedSafeString<MESSAGESIZE>* tryGetMessage();
    static bool shouldKids() { return sInstance ? sInstance->isKids : false; }
    static u8 getHealth() { return sInstance ? sInstance->mHealth : 3; }
    static int getCoins() { return sInstance ? sInstance->mCoins : 0; }
    static bool isNeedUpdateHealthCoins() { return sInstance ? sInstance->mNeedsUpdateHealthCoins : false; }
    static void setNeedUpdateHealthCoins(bool value);
    static void setServerVersion(const char* serverVersion);
    static const char* getServerVersion();

    // ===== SERVER CONFIGURATION =====
    static const int getCurrentPort();
    static const char* getCurrentIP();
    static const bool hasServerChanged();
    static void setLastUsedIP(const char* ip);
    static void setLastUsedPort(const int port);
    static void setServerIP(const char* ip);
    static void setServerPort(int port);

    // ===== SERVER VISIBILITY =====
    static bool isServerHidden() { return sInstance ? sInstance->mServerHidden : true; }
    static void setServerHidden(bool hide) {
        if (sInstance) {
            sInstance->mServerHidden = hide;
        }
    }
    static void toggleServerHidden() {
        if (sInstance) {
            sInstance->mServerHidden = !sInstance->mServerHidden;
        }
    }

    // ===== MUSIC SETTINGS =====
    static bool isMusicDisabled() { return sInstance->mIsDisableMusic; }
    static void toggleMusicDisabled() {
        if (sInstance) {
            sInstance->mIsDisableMusic = !sInstance->mIsDisableMusic;
        }
    }

    // ===== UTILITY METHODS =====
    static void update();
    static void clearArrays();
    static sead::Heap* getClientHeap() { return sInstance ? sInstance->mHeap : nullptr; }
    static Keyboard* getKeyboard();

    // ===== STAGE MANAGEMENT =====
    static void setStageInfo(GameDataHolderAccessor holder);
    static void setSceneInfo(const al::ActorInitInfo& initInfo, const StageScene* stageScene);
    static void setTagState(bool state);

    // ===== UI METHODS =====
    static bool openKeyboardIP();
    static bool openKeyboardPort();
    static void showUIMessage(const char16_t* msg);
    static void hideUIMessage();
    static void showConnect();
    static void showConnectError(const char16_t* msg);
    static void hideConnect();

    // ===== PUBLIC MEMBERS (for debug purposes) =====
    SocketClient* mSocket;

    PlayerInf* getLastPlayerInfPacket() { return &this->lastPlayerInfPacket; }
    GameInf* getLastGameInfPacket() { return &this->lastGameInfPacket; }
    CostumeInf* getLastCostumeInfPacket() { return &this->lastCostumeInfPacket; }
    CaptureInf* getLastCaptureInfPacket() { return &this->lastCaptureInfPacket; }

    static al::Sequence* getSequence() { return sInstance ? sInstance->mSequence : nullptr; }

    static void setSequence(al::Sequence* sequence) {
        if (sInstance) {
            sInstance->mSequence = sequence;
        }
    }

    bool mIsAllowReconnect = false;

    // ===== Message System =====
    int getMsgCount() { return mMessageQueue.mMessageQueueInner._count; };
    static int getMaxMsgCount() { return sMaxMsgCount; };

private:
    // ===== CORE FUNCTIONALITY =====
    void readFunc();
    bool startConnection();

    // ===== PACKET HANDLERS =====
    void updatePlayerInfo(PlayerInf* packet);
    void updateHackCapInfo(HackCapInf* packet);
    void updateGameInfo(GameInf* packet);
    void updateCostumeInfo(CostumeInf* packet);
    void updateShineInfo(ShineCollect* packet);
    void updatePlayerConnect(PlayerConnect* packet);
    void updateTagInfo(TagInf* packet);
    void updateFreezeInfo(FreezeInf* packet);
    void handleFreezeInfRoundPacket(FreezeInfRoundPacket* packet);
    void updateShineThiefInfo(ShineThiefInf* packet);
    void handleShineThiefRoundPacket(ShineThiefInfRoundPacket* packet);
    void updateCaptureInfo(CaptureInf* packet);
    void sendToStage(ChangeStagePacket* packet);
    void disconnectPlayer(PlayerDC* packet);
    void updateMessages(MessagePacket* packet);
    void updateHealthCoins(HealthCoins* packet);
    void updateCoinCollects(CoinCollectCollect* packet);

    // ===== UTILITY METHODS =====
    PuppetInfo* findPuppetInfo(const nn::account::Uid& id, bool isFindAvailable);

    /**
     * @brief Core logic for applying a coin collect to game state and killing the actor.
     *        Called both from updateCoinCollects (when scene is ready) and from update()
     *        when draining mPendingCoinCollects.
     */
    static void applyOneCoinCollect(const char* placeID, int worldID, const char* stage);

    // ===== CONNECTION MEMBERS =====
    al::AsyncFunctorThread* mReadThread = nullptr;
    int mConnectCount = 0;
    nn::account::Uid mUserID;
    sead::FixedSafeString<0x20> mUsername;
    bool mIsConnectionActive = false;
    bool mIsFirstConnect = true;
    bool waitForGameInit = true;

    // ===== SERVER CONFIGURATION MEMBERS =====
    hostname mServerIP;
    int mServerPort = 0;
    sead::FixedSafeString<64> mServerVersion;
    bool mServerHidden = true;
    bool mIsDisableMusic = false;

    // ===== HEALTH AND COINS =====
    bool isKids = false;
    u8 mHealth = 3;
    int mCoins = 0;
    bool mNeedsUpdateHealthCoins = false;

    // ===== SHINE SYNCHRONIZATION MEMBERS =====
    sead::SafeArray<int, 128> curCollectedShines;
    int collectedShineCount = 0;
    int lastCollectedShine = -1;

    // ===== COIN COLLECT PENDING QUEUE =====
    // Coin collect packets that arrived while mCurStageScene was null are stored here
    // and applied in update() once the scene becomes available.
    static constexpr int sMaxPendingCoinCollects = 50;
    PendingCoinCollect mPendingCoinCollects[sMaxPendingCoinCollects];
    int mPendingCoinCollectCount = 0;

    // ===== MESSAGE MEMBERS =====
    static const int sMaxMsgCount = 100;
    sead::MessageQueue mMessageQueue;

    // ===== PACKET BACKUPS =====
    PlayerInf lastPlayerInfPacket = PlayerInf();
    GameInf lastGameInfPacket = GameInf();
    GameInf emptyGameInfPacket = GameInf();
    CostumeInf lastCostumeInfPacket = CostumeInf();
    CaptureInf lastCaptureInfPacket = CaptureInf();

    // ===== UI COMPONENTS =====
    Keyboard* mKeyboard = nullptr;
    al::WindowConfirmWait* mUIMessage;
    al::SimpleLayoutAppearWaitEnd* mConnectStatus;

    // ===== GAME STATE MEMBERS =====
    bool isClientCaptured = false;
    bool isSentCaptureInf = false;
    bool isSentHackInf = false;

    // ===== SCENE AND STAGE MEMBERS =====
    al::ActorSceneInfo* mSceneInfo = nullptr;
    const StageScene* mCurStageScene = nullptr;
    sead::PtrArray<Shine> mShineArray;
    sead::PtrArray<CoinCollect> mCoinCollectArray;
    sead::PtrArray<CoinCollect2D> mCoinCollect2DArray;
    sead::FixedSafeString<0x40> mStageName;
    GameDataHolderAccessor mHolder;
    u8 mScenario = 0;

    // ===== MEMORY MANAGEMENT =====
    sead::ExpHeap* mHeap = nullptr;

    // ===== PUPPET MANAGEMENT MEMBERS =====
    int maxPuppets = 9;
    PuppetInfo* mPuppetInfoArr[MAXPUPINDEX] = {};
    PuppetHolder* mPuppetHolder = nullptr;
    PuppetInfo mDebugPuppetInfo;

    // ===== SEQUENCE =====
    al::Sequence* mSequence = nullptr;  // current sequence, used for debug menu
};