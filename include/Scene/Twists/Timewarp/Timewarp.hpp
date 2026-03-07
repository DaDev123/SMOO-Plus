#pragma once

#include "hk/ro/RoUtil.h"

#include "sead/container/seadPtrArray.h"
#include "sead/gfx/seadColor.h"
#include "sead/gfx/seadPrimitiveRenderer.h"
#include "sead/math/seadVector.h"
#include "sead/prim/seadSafeString.h"

#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Scene/StageScene.h"

class TimeFrameCap {
public:
    bool isFlying = false;
    sead::Vector3f position = sead::Vector3f::zero;
    sead::Vector3f rotation = sead::Vector3f::zero;
    sead::FixedSafeString<0x20> action;
};

class TimeFramePlayer {
public:
    sead::Vector3f velocity = sead::Vector3f::zero;
    sead::Vector3f gravity = -1 * sead::Vector3f::ey;
    sead::Quatf rotation;
    sead::FixedSafeString<0x20> action;
    float actionFrame = 0.f;
};

class TimeFrame {
public:
    float colorFrame = 0.f;
    sead::Vector3f position = sead::Vector3f::zero;
    TimeFramePlayer playerFrame;
    TimeFrameCap capFrame;
};

class TimeWarpTwist {
public:
    static bool sTimeWarpEnabled;
    static bool isTimeWarpEnabled() { return sTimeWarpEnabled; }
    static void toggleTimeWarp();

    static void init();                           // call once at boot (also installs hooks)
    static void initHooks();                      // installs oxygen ring hooks
    static void onStageInit(StageScene* scene);   // call on every stage init
    static void onStageDeath();                   // call on player death / scene kill
    static void update(PlayerActorHakoniwa* p1);  // call every frame (skipped when disabled)

    static void drawTrail(al::Scene* scene, sead::PrimitiveRenderer* renderer);

    static int getTimeArraySize();
    static float getColorFrame();
    static float getCooldownTimer();
    static int getRewindDelay();
    static bool isSceneActive();
    static bool isRewind();
    static bool isOnCooldown();

    static TimeFrame* getTimeFrame(u32 index);
    static sead::Color4f calcColorFrame(float colorFrame, int dotIndex);
    static sead::Vector3f calcDotTrans(sead::Vector3f position, int dotIndex);
    static float calcCooldownPercent();

private:
    static bool sIsRewinding;
    static bool sIsCapture;
    static bool sIsCaptureInvalid;
    static bool sIs2D;
    static int sSceneInactiveTime;

    static constexpr int maxFrames = 400;
    static sead::PtrArray<TimeFrame> sTimeFrames;

    static int sRewindFrameDelay;
    static int sRewindFrameDelayTarget;
    static constexpr int minTrailLength = 40;
    static constexpr float minPushDistance = 20.f;

    static float sColorFrame;
    static constexpr float colorFrameRate = 0.05f;
    static float sColorFrameOffset;
    static constexpr float colorFrameOffsetRate = 0.07f;
    static int sDotBounceIndex;

    static bool sIsCooldown;
    static float sCooldownCharge;
    static constexpr float cooldownRate = 0.15f;
    static constexpr float cooldownDischarge = 0.3f;

    static StageScene* sStageScene;

    static void pushNewFrame();
    static void rewindFrame(PlayerActorHakoniwa* p1);
    static void updateHackCap(HackCap* cap, al::LiveActor* headModel);
    static void startRewind(PlayerActorHakoniwa* p1);
    static void endRewind(PlayerActorHakoniwa* p1);
    static void setPostProcessingId(int id);
    static void emptyFrameInfo();
    static void resetCooldown();
    static bool isInvalidCapture(const char* curName);
};