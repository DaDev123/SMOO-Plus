#include "cameras/CameraPoserActorSpectate.h"
#include "al/alCollisionUtil.h"
#include "al/camera/CameraPoser.h"
#include "al/camera/alCameraPoserFunction.h"
#include "al/util.hpp"
#include "al/util/ControllerUtil.h"
#include "al/util/LiveActorUtil.h"
#include "al/util/MathUtil.h"
#include "al/util/VectorUtil.h"
#include "logger.hpp"
#include "math/seadMathCalcCommon.h"
#include "sead/gfx/seadCamera.h"
#include "sead/math/seadVector.h"

namespace cc {

CameraPoserActorSpectate::CameraPoserActorSpectate(const char* poserName)
    : CameraPoser(poserName)
{
    this->initOrthoProjectionParam();
}

void CameraPoserActorSpectate::init(void)
{
    alCameraPoserFunction::initSnapShotCameraCtrlZoomRollMove(
        this); // this makes the snapshot camera have the abilities of the normal snapshot cam, but
               // is locked rotationally
    alCameraPoserFunction::initCameraVerticalAbsorber(this);
    alCameraPoserFunction::initCameraAngleCtrl(this);
}

void CameraPoserActorSpectate::start(al::CameraStartInfo const&)
{
    if (mTargetActorPos)
        mTargetTrans = *mTargetActorPos;

    if (mPlayer && !mTargetActorPos)
        mTargetTrans = *al::getTransPtr(mPlayer);

    mFovyDegree = 47.5f;
    mFrameCounter = 0;

    mPoserFlags->mOffVerticalAbsorb = true;
    mPoserFlags->mInvalidChangeSubjective = true;
    mPoserFlags->mInvalidPreCameraEndAfterInterpole = true;
    mPoserFlags->mInvalidChangeSubjective = true;
    mPoserFlags->mValidKeepPreSelfPoseNextCameraByParam = false;
}

void CameraPoserActorSpectate::movement()
{
    mFrameCounter++;

    if (mTargetActorPos)
        al::lerpVec(&mTargetTrans, mTargetTrans - sead::Vector3f(0.f, 100.f, 0.f), *mTargetActorPos, mFrameCounter >= 10 ? 0.08f : 1.f);

    if (mPlayer && !mTargetActorPos)
        mTargetTrans = *al::getTransPtr(mPlayer);

    if (!mPlayer && !mTargetActorPos)
        return;

    mTargetTrans.y += mYOffset;

    // calculates the targets direction through only the X and Z axis
    sead::Vector3f targetDir = sead::Vector3f(mPosition.x - mTargetTrans.x, 0.0f, mPosition.z - mTargetTrans.z);
    al::tryNormalizeOrDirZ(&targetDir);

    sead::Vector3f rotatedVec;
    sead::Vector3f rightVec;

    calcRotVec(targetDir, &rotatedVec, &rightVec);

    float maxDist = mDistMax; // This will be cut short if a solid surface is found in the way
    sead::Vector3f resultVec;
    bool isHit = false;
    if (mPlayer)
        isHit = alCollisionUtil::getFirstPolyOnArrow(mPlayer, &resultVec, nullptr, mTargetTrans, rotatedVec * mDistMax, nullptr, nullptr);

    if (isHit && mTargetActorPos) {
        // Formula calculates distance between result vec and target trans
        maxDist = sqrt(pow((resultVec.x - mTargetTrans.x), 2)
            + pow((resultVec.y - mTargetTrans.y), 2)
            + pow((resultVec.z - mTargetTrans.z), 2));

        maxDist -= maxDist / 30.f;
        if (maxDist <= 1)
            maxDist = 1.f;
    }

    if (maxDist > mDistLerp)
        mDistLerp = al::lerpValue(mDistLerp, maxDist, 0.07f);
    else
        mDistLerp = maxDist;

    mPosition = mTargetTrans + (rotatedVec * mDistLerp);
}

void CameraPoserActorSpectate::calcRotVec(sead::Vector3f targetDir, sead::Vector3f* rotatedVec, sead::Vector3f* rightVec)
{
    sead::Vector2f playerRInput(0, 0);
    alCameraPoserFunction::calcCameraRotateStick(&playerRInput, this);
    mInterpRStick.x = al::lerpValue(mInterpRStick.x, playerRInput.x, 0.2f);
    mInterpRStick.y = al::lerpValue(mInterpRStick.y, playerRInput.y, 0.2f);

    // rotates target direction by the cameras X input
    al::rotateVectorDegreeY(&targetDir, (mInterpRStick.x > 0.0f ? mInterpRStick.x : -mInterpRStick.x) < 0.02f ? 0.0f : mInterpRStick.x * -2.0f);

    mAngle += mInterpRStick.y * -2.0f;
    mAngle = al::clamp(mAngle, -89.0f, 89.0f);

    *rotatedVec = targetDir;

    // calculates cross product of target direction and cameras current up direction
    sead::Vector3f crossVec;
    crossVec.setCross(targetDir, mCameraUp);
    // rotates target direction by the cross product and vertical angle
    al::rotateVectorDegree(rotatedVec, *rotatedVec, crossVec, mAngle);

    // Calculate the camera's right
    *rightVec = *rotatedVec;
    al::rotateVectorDegreeY(rightVec, 90.f);
}

} // namespace cc