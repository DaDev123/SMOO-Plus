#pragma once

#include "al/Library/Scene/SceneUtil.h"
#include "al/Library/Thread/AsyncFunctorThread.h"

#include "game/Player/PlayerActorHakoniwa.h"

#include "layouts/ShineThiefIcon.h"
#include "math/seadVectorFwd.h"
#include "packets/ShineThiefInf.h"
#include "Player/PlayerConst.h"
#include "puppets/PuppetInfo.h"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/shine-thief/ShineThiefHintArrow.h"
#include "server/shine-thief/ShineThiefInfo.h"
#include "server/shine-thief/ShineThiefPlayerBlock.h"
#include "server/shine-thief/ShineThiefScore.hpp"
#include "server/shine-thief/StageBoundInfo.h"

struct OriginalPlayerConst {
    f32 mNormalMaxSpeed;
    f32 mNormalMaxSpeed2D;
    f32 mNormalMinSpeed;
    f32 mNormalMinSpeed2D;
    f32 mDashBorderSpeed;
    f32 mDashFastBorderSpeed;
    f32 mDashMaxSpeed2D;
    f32 mSlopeRollingMaxSpeed;
    f32 mSlopeRollingSideMaxSpeed;
    f32 mSlopeRollingSpeedBoost;
    f32 mSlopeRollingSpeedEnd;
    f32 mSlopeRollingSpeedStart;
    f32 mLongJumpSpeed;
    f32 mLongJumpSpeedMin;
    f32 mLongJumpInitSpeed;
};

class ShineThiefMode : public GameModeBase {
public:
    ShineThiefMode(const char* name);

    void init(GameModeInitInfo const& info) override;
    void begin() override;
    void update() override;
    void end() override;
    void pause() override;
    void unpause() override;
    void debugMenuControls() override;

    bool isUseNormalUI() const override { return false; }

    // Packet management
    void sendShineThiefPacket(ShineThiefUpdateType updateType);
    ShineThiefUpdateType getNextUpdateType() const { return mNextUpdateType; }

    // Round management
    void startRound(int roundMinutes);
    void endRound(bool isAbort);
    void placeShine();
    void initBounds();

    // State queries
    bool isScoreEventsEnabled() const { return mIsScoreEventsValid; }
    bool isPlayerHolder() const { return mInfo->mIsPlayerHolder; }
    float getInvulnTime() const { return mInvulnTime; }
    bool isEndgameActive() { return mIsEndgameActive; }
    ShineThiefInfo* getInfo() const { return mInfo; }

    // Player state management
    bool trySetPlayerHolderState(bool hasShine);
    bool tryStealShine();
    void forceDropShine();

    // Events
    void tryStartEndgameEvent();
    bool tryStartRecoveryEvent(bool isEndgame);
    bool tryEndRecoveryEvent();
    void tryScoreEvent(ShineThiefInf* incomingPacket, PuppetInfo* sourcePuppet);

    // Utilities
    PlayerActorHakoniwa* getPlayerActorHakoniwa();
    void warpToRecoveryPoint(PlayerActorHakoniwa* actor);
    bool trySetPostProcessingType(ShineThiefPostProcessingType type);
    void setWipeHolder(al::WipeHolder* wipe) { mWipeHolder = wipe; }
    sead::Vector3f getShinePos() { return mInfo->shinePos; };
    void setShinePos(sead::Vector3f pos) { mInfo->shinePos.set(pos); };
    ShineThiefPlayerBlock* getShineBlock() { return mMainPlayerCaptureBlock; };

    void setCameraTicket(al::CameraTicket* ticket) { mSpectateTicket = ticket; }
    void updateIntroCamera();

    void updatePlayerConst();
    sead::Vector3f getHostStartPos() const { return mHostStartPos; }
    void setHostStartPos(const sead::Vector3f& pos) { mHostStartPos = pos; }

    ShineThiefIcon* getLayout() { return mModeLayout; }

private:
    // Round intro frame-based updates
    void updateRoundIntro();

    ShineThiefInfo* mInfo = nullptr;
    GameModeTimer* mModeTimer = nullptr;
    ShineThiefIcon* mModeLayout = nullptr;
    al::WipeHolder* mWipeHolder = nullptr;
    StageBoundInfo* mBoundInfo = nullptr;
    al::CameraTicket* mSpectateTicket = nullptr;

    // Visual actors
    ShineThiefPlayerBlock* mMainPlayerCaptureBlock = nullptr;
    ShineThiefHintArrow* mHintArrow = nullptr;

    // State
    ShineThiefUpdateType mNextUpdateType = ShineThiefUpdateType::PLAYER;
    float mInvulnTime = 0.0f;
    bool mIsScoreEventsValid = false;

    // Endgame
    bool mIsEndgameActive = false;
    float mEndgameTimer = -1.f;

    // Round intro animation state
    bool mIsRoundIntro = false;
    int mRoundIntroFrames = 0;

    // Recovery
    int mRecoveryEventFrames = 0;
    const int mRecoveryEventLength = 60;
    sead::Vector3f mRecoverySafetyPoint = sead::Vector3f::zero;

    float mShineHoldTimer = 0.f;         // Tracks how long player has held shine
    bool mIsConstApplied = false;        // Tracks if ShineThief const is currently applied
    OriginalPlayerConst mOriginalConst;  // Stores original const values for restoration

    sead::Vector3f mHostStartPos = sead::Vector3f::zero;
};