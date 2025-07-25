#pragma once

#include "al/LiveActor/LiveActor.h"
#include "al/camera/CameraPoser.h"
#include "al/camera/alCameraPoserFunction.h"

#include "game/Player/PlayerActorBase.h"
#include "sead/math/seadVector.h"

#include "al/util.hpp"

// cc = custom cameras

namespace cc {
class CameraPoserActorSpectate : public al::CameraPoser {
public:
    CameraPoserActorSpectate(char const*);
    virtual void start(al::CameraStartInfo const&) override;
    virtual void init() override;
    void reset(void);
    virtual void update(void) override;
    virtual void movement(void) override;

    void setTargetActor(sead::Vector3f* target) { mTargetActorPos = target; }
    void setPlayer(PlayerActorBase* base) { mPlayer = base; };

    void calcRotVec(sead::Vector3f targetDir, sead::Vector3f* rotatedVec, sead::Vector3f* rightVec);

    float mAngle = 20.f;
    float mDistMax = 1400.f;
    float mDistLerp = 1400.f;
    float mYOffset = 100.f;
    float mDefaultFovy = 35.f;
    int mFrameCounter = 0;

    sead::Vector2f mInterpRStick = sead::Vector2f::zero;

private:
    sead::Vector3f* mTargetActorPos = nullptr;
    PlayerActorBase* mPlayer = nullptr;
};
}