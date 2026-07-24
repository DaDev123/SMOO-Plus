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
#include "al/Library/LiveActor/ActorInitInfo.h"
#include "al/Library/Play/Layout/SimpleLayoutAppearWaitEnd.h"
#include "al/Library/Sequence/Sequence.h"
#include "al/Library/Thread/AsyncFunctorThread.h"
#include "al/Library/Yaml/Writer/ByamlWriter.h"

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
#include "sead/container/seadSafeArray.h"
#include "sead/heap/seadDisposer.h"
#include "sead/prim/seadSafeString.h"

#include "container/seadPtrArray.h"
#include "Library/Yaml/ByamlIter.h"
#include "Sequence/HakoniwaSequence.h"

// ===== PROJECT INCLUDES =====
#include "Keyboard.hpp"
#include "puppets/PuppetHolder.hpp"
#include "puppets/PuppetInfo.h"
#include "server/SocketClient.hpp"

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
    sead::FixedSafeString<0x40> placeID;
    int worldID;
    sead::FixedSafeString<0x40> stage;
};

struct PendingCheckpoint {
    sead::FixedSafeString<0x40> objId;
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

    // ==== MOON ROCK MANAGEMENT ====
    void saveMoonRocks(al::ByamlWriter* writer);
    void readMoonRocks(const al::ByamlIter& save);

    // ===== PACKET SENDING METHODS =====
    static void sendHackCapInfPacket(const HackCap* hackCap);
    static void sendPlayerInfPacket(const PlayerActorBase* player, bool isYukimaru);
    static void sendGameInfPacket(const PlayerActorHakoniwa* player, GameDataHolderAccessor holder);
    static void sendGameInfPacket(GameDataHolderAccessor holder);
    // static void sendCaptureInfPacket(const PlayerActorHakoniwa* player);
    static void sendCostumeInfPacket(const char* body, const char* cap);
    static void sendShineCollectPacket(int shineId);
    static void sendCoinCollectCollectPacket(const char* placeID, int worldID, const char* stage);
    static void sendCheckpointGetPacket(const char* objId);
    static void sendMoonRockHitPacket(int worldId);
    static void sendGameStartPacket();

    // ===== PUPPET MANAGEMENT =====
    static bool tryAddPuppet(PuppetActor* puppet);

    static PuppetActor* getPuppet(int idx);
    static PuppetInfo* getPuppetInfo(int idx);
    static PuppetInfo* getLatestInfo();
    static PuppetHolder* getPuppetHolder() {
        if (sInstance)
            return sInstance->mPuppetHolder;
        return nullptr;
    }

    // ===== CLIENT INFO GETTERS =====
    static const char* getClientName() { return sInstance ? sInstance->mUsername.cstr() : "Player"; }
    static nn::account::Uid getClientId() { return sInstance ? sInstance->mUserID : nn::account::Uid(); }
    static sead::FixedSafeString<0x20> getUsername() {
        return sInstance ? sInstance->mUsername : sead::FixedSafeString<0x20>::cEmptyString;
    }
    // static bool shouldKids() { return sInstance ? sInstance->isKids : false; }
    // static u8 getHealth() { return sInstance ? sInstance->mHealth : 3; }
    // static int getCoins() { return sInstance ? sInstance->mCoins : 0; }
    // static bool isNeedUpdateHealthCoins() { return sInstance ? sInstance->mNeedsUpdateHealthCoins : false;
    // } static void setNeedUpdateHealthCoins(bool value);

    static bool isThreadDone() { return sInstance ? sInstance->mReadThread->isDone() : true; }

    // ===== SERVER CONFIGURATION =====
    static const int getCurrentPort();
    static const char* getCurrentIP();
    static bool hasServerChanged();
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

    // ===== RUMBLE =====
    static bool shouldStopRumble() { return sInstance ? sInstance->mShouldStopRumble : false; }
    static void clearStopRumble() {
        if (sInstance)
            sInstance->mShouldStopRumble = false;
    }
    static void setStopRumble() {
        if (sInstance)
            sInstance->mShouldStopRumble = true;
    }
    // ===== UTILITY METHODS =====
    static void update();
    static void clearArrays();
    static Keyboard* getKeyboard();
    static PuppetInfo* findPuppetInfo(const nn::account::Uid& id, bool isFindAvailable);

    // ===== STAGE MANAGEMENT =====
    static void setStageInfo(HakoniwaSequence* sequence);
    static void setTagState(bool state);

    // ===== UI METHODS =====
    static bool openKeyboardIP();
    static bool openKeyboardPort();

    // ===== PUBLIC MEMBERS (for debug purposes) =====
    SocketClient* mSocket;

    PlayerInf* getLastPlayerInfPacket() { return &this->lastPlayerInfPacket; }
    GameInf* getLastGameInfPacket() { return &this->lastGameInfPacket; }
    CostumeInf* getLastCostumeInfPacket() { return &this->lastCostumeInfPacket; }
    // CaptureInf* getLastCaptureInfPacket() { return &this->lastCaptureInfPacket; }

    static al::Sequence* getSequence() { return sInstance ? sInstance->mSequence : nullptr; }

    static void setSequence(al::Sequence* sequence) {
        if (sInstance) {
            sInstance->mSequence = sequence;
        }
    }

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
    // void updateCaptureInfo(CaptureInf* packet);
    void sendToStage(ChangeStagePacket* packet);
    void disconnectPlayer(PlayerDC* packet);
    void updateCoinCollects(CoinCollectCollect* packet);
    void updateCheckpoints(CheckpointGet* packet);
    void updateMoonRocks(MoonRockHit* packet);

    // ===== UTILITY METHODS =====

    /**
     * @brief Core logic for applying a coin collect to game state and killing the actor.
     *        Called both from updateCoinCollects (when scene is ready) and from update()
     *        when draining mPendingCoinCollects.
     */
    static void applyOneCoinCollect(const char* placeID, int worldID, const char* stage);
    static void getOneCheckpoint(const char* objId);

    // ===== CONNECTION MEMBERS =====
    al::AsyncFunctorThread* mReadThread = nullptr;
    int mConnectCount = 0;
    bool mShouldStopRumble = false;
    nn::account::Uid mUserID;
    sead::FixedSafeString<0x20> mUsername;
    bool mIsConnectionActive = false;
    bool mIsFirstConnect = true;

    // ===== SERVER CONFIGURATION MEMBERS =====
    hostname mServerIP;
    int mServerPort = 0;
    sead::FixedSafeString<64> mServerVersion;
    bool mServerHidden = true;
    bool mIsDisableMusic = false;

    // ===== SHINE SYNCHRONIZATION MEMBERS =====
    sead::SafeArray<int, 128> curCollectedShines;
    int collectedShineCount = 0;
    int lastCollectedShine = -1;

    // ===== COIN COLLECT PENDING QUEUE =====
    // Coin collect packets that arrived while mCurStageScene was null are stored here
    // and applied in update() once the scene becomes available.
    static constexpr s32 sMaxPendingCoinCollects = 50;
    sead::SafeArray<PendingCoinCollect, sMaxPendingCoinCollects> mPendingCoinCollects;
    s32 mPendingCoinCollectCount = 0;

    // ===== CHECKPOINT PENDING QUEUE =====
    // Checkpoint get packets that arrived while mCurStageScene was null are stored here
    // and applied in update() once the scene becomes available.
    static constexpr s32 sMaxPendingCheckpoints = 10;
    sead::SafeArray<PendingCheckpoint, sMaxPendingCheckpoints> mPendingCheckpoints;
    s32 mPendingCheckpointCount = 0;

    // ===== MOON ROCK PENDING QUEUE =====
    // Moon Rocks when players haven't beaten the game yet get stored here.
    // The moon rock scenario is applied when the game is beaten.
    static constexpr s32 SNumMoonRocks = 14;
    sead::SafeArray<bool, SNumMoonRocks> mPendingMoonRocks;

    // ===== PACKET BACKUPS =====
    PlayerInf lastPlayerInfPacket = PlayerInf();
    GameInf lastGameInfPacket = GameInf();
    GameInf emptyGameInfPacket = GameInf();
    CostumeInf lastCostumeInfPacket = CostumeInf();
    // CaptureInf lastCaptureInfPacket = CaptureInf();

    // ===== UI COMPONENTS =====
    Keyboard* mKeyboard = nullptr;
    al::SimpleLayoutAppearWaitEnd* mConnectStatus = nullptr;

    // ===== GAME STATE MEMBERS =====
    bool isClientCaptured = false;
    bool isSentCaptureInf = false;
    bool isSentHackInf = false;

    // ===== SCENE AND STAGE MEMBERS =====
    const StageScene* mCurStageScene = nullptr;
    sead::PtrArray<Shine> mShineArray;
    sead::PtrArray<CoinCollect> mCoinCollectArray;
    sead::PtrArray<CoinCollect2D> mCoinCollect2DArray;
    sead::FixedSafeString<0x40> mStageName;
    GameDataHolderAccessor mHolder;
    u8 mScenario = 0;

    // ===== PUPPET MANAGEMENT MEMBERS =====
    int maxPuppets = 9;
    PuppetInfo* mPuppetInfoArr[MAXPUPINDEX] = {};
    PuppetHolder* mPuppetHolder = nullptr;

    // ===== SEQUENCE =====
    al::Sequence* mSequence = nullptr;  // current sequence, used for debug menu
};