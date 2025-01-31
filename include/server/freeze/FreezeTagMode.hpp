#pragma once

#include "al/camera/CameraTicket.h"
#include "container/seadPtrArray.h"
#include "container/seadSafeArray.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/StageScene/StageScene.h"
#include "layouts/FreezeTagIcon.h"
#include "math/seadVector.h"
#include "puppets/PuppetInfo.h"
#include "server/freeze/FreezeHintArrow.h"
#include "server/freeze/FreezePlayerBlock.h"
#include "server/freeze/FreezeTagInfo.h"
#include "server/freeze/FreezeTagScore.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeConfigMenu.hpp"
#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/hns/HideAndSeekConfigMenu.hpp"
#include <math.h>
#include <stdint.h>

enum FreezeUpdateType : u8 { // Type of packets to send between players
    PLAYER                 = 1 << 0,
    ROUNDSTART             = 1 << 1,
    ROUNDCANCEL            = 1 << 2,
    FALLOFF                = 1 << 3
};

enum FreezePostProcessingType : u8 { // Snapshot mode post processing state
    PPDISABLED = 0,
    PPFROZEN = 1,
    PPENDGAMELOSE = 2,
    PPENDGAMEWIN = 3
};

struct PACKED FreezeTagPacket : Packet {
    FreezeTagPacket() : Packet() { this->mType = PacketType::GAMEMODEINF; mPacketSize = sizeof(FreezeTagPacket) - sizeof(Packet);};
    FreezeUpdateType updateType;
    bool isRunner = false;
    bool isFreeze = false;
    uint16_t score = 0;
};

struct PACKED FreezeTagRoundPacket : Packet {
    FreezeTagRoundPacket() : Packet() { this->mType = PacketType::GAMEMODEINF; mPacketSize = sizeof(FreezeTagPacket) - sizeof(Packet);};
    FreezeUpdateType updateType;
    uint8_t roundTime = 10;
    const char padding[3] = "\0\0";
};

class FreezeTagMode : public GameModeBase {
public:
    FreezeTagMode(const char* name);

    void init(GameModeInitInfo const& info) override;

    virtual void begin() override;
    virtual void update() override;
    virtual void end() override;

    void pause() override;
    void unpause() override;

    bool isUseNormalUI() const override { return false; }

    void processPacket(Packet* packet) override;
    Packet* createPacket() override;
    void sendFreezePacket(FreezeUpdateType updateType); // Called instead of Client::sendGamemodePacket(), allows setting packet type

    void startRound(int roundMinutes); // Actives round on this specific client
    void endRound(bool isAbort); // Ends round, allows setting for if this was a natural end or abort (used for scoring)

    bool isScoreEventsEnabled() const { return mIsScoreEventsValid; };
    bool isPlayerRunner() const { return mInfo->mIsPlayerRunner; };
    bool isPlayerFreeze() const { return mInfo->mIsPlayerFreeze; };
    bool isEndgameActive() { return mIsEndgameActive; }  // The endagme is the time during the WIPEOUT message is on screen
    bool isPlayerLastSurvivor(PuppetInfo* changingPuppet); // Only meant to be called on getting a packet
    bool isAllRunnerFrozen(PuppetInfo* changingPuppet); // Only meant to be called on getting a packet, starts the endgame

    PlayerActorHakoniwa* getPlayerActorHakoniwa(); // Returns nullptr if the player is not a PlayerActorHakoniwa
    uint16_t getScore() { return mInfo->mPlayerTagScore.mScore; }

    bool trySetPlayerRunnerState(FreezeState state); // Sets runner to alive or frozen, many safety checks
    void tryStartEndgameEvent(); // Starts the WIPEOUT message event
    bool tryStartRecoveryEvent(bool isEndgame); // Returns player to a chaser's position or last stood position, unless endgame variant
    bool tryEndRecoveryEvent(); // Called after the fade of the recovery event
    void tryScoreEvent(FreezeTagPacket* incomingPacket, PuppetInfo* sourcePuppet); // Attempt score gain when getting a packet
    void setWipeHolder(al::WipeHolder* wipe) { mWipeHolder = wipe; }; // Called with HakoniwaSequence hook, wipe used in recovery event
    bool trySetPostProcessingType(FreezePostProcessingType type); // Sets the post processing type, also used for disabling
    
    void warpToRecoveryPoint(al::LiveActor* actor); // Warps runner to chaser OR if impossible, last standing position

    void updateSpectateCam(PlayerActorBase* playerBase); // Updates the frozen spectator camera
    void setCameraTicket(al::CameraTicket* ticket) { mTicket = ticket; } // Called when the camera ticket is constructed to get a pointer

private:
    FreezeUpdateType mNextUpdateType = FreezeUpdateType::PLAYER; // Set for the sendPacket funtion to know what packet type is sent
    FreezePostProcessingType mPostProcessingType = FreezePostProcessingType::PPDISABLED; // Current post processing mode (snapshot mode)
    GameModeTimer* mModeTimer = nullptr; // Generic timer from H&S used for round timer
    FreezeTagIcon* mModeLayout = nullptr; // HUD layout (creates sub layout actors for runner and chaser)
    FreezeTagInfo* mInfo = nullptr;
    al::WipeHolder* mWipeHolder = nullptr; // Pointer set by setWipeHolder on first step of hakoniwaSequence hook

    // Scene actors
    FreezePlayerBlock* mMainPlayerIceBlock = nullptr; // Visual block around player's when frozen
    FreezeHintArrow* mHintArrow = nullptr; // Arrow that points to nearest runner while in chaser

    // Recovery event info
    int mRecoveryEventFrames = 0;
    const int mRecoveryEventLength = 60; // Length of recovery event in frames
    sead::Vector3f mRecoverySafetyPoint = sead::Vector3f::zero;

    // Endgame info
    bool mIsEndgameActive = false;
    float mEndgameTimer = -1.f;
    
    float mInvulnTime = 0.0f;
    bool mIsScoreEventsValid = false;

    // Spectate camera ticket and target information
    al::CameraTicket* mTicket = nullptr;
    int mPrevSpectateIndex = -2;
    int mSpectateIndex = -1;
};