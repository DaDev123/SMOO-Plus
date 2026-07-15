#include "helpers.hpp"

#include "hk/diag/diag.h"
#include "hk/util/Algorithm.h"

#include "al/Library/LiveActor/ActorMovementFunction.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/Math/MathUtil.h"
#include "al/Library/Player/PlayerUtil.h"
#include "al/Library/Scene/Scene.h"
#include "al/Library/Sequence/Sequence.h"

#include "game/Player/PlayerAnimator.h"
#include "game/Scene/StageScene.h"
#include "game/Sequence/HakoniwaSequence.h"
#include "game/System/GameDataFunction.h"
#include "game/System/GameSystem.h"

#include "System/GameDataHolderWriter.h"

bool isPartOf(const char* w1, const char* w2) {
    int i = 0;
    int j = 0;

    if (strlen(w1) <= 0) {
        return false;
    }

    while (w1[i] != '\0') {
        if (w1[i] == w2[j]) {
            int init = i;
            while (w1[i] == w2[j] && w2[j] != '\0') {
                j++;
                i++;
            }
            if (w2[j] == '\0') {
                return true;
            }
            j = 0;
        }
        i++;
    }
    return false;
}

int indexOf(char* w1, char c1) {
    for (int i = 0; i < strlen(w1); i++) {
        if (w1[i] == c1) {
            return i;
        }
    }
    return -1;
}

sead::Vector3f QuatToEuler(sead::Quatf* quat) {
    f32 x = quat->z;
    f32 y = quat->y;
    f32 z = quat->x;
    f32 w = quat->w;

    f32 t0 = 2.0 * (w * x + y * z);
    f32 t1 = 1.0 - 2.0 * (x * x + y * y);
    f32 roll = atan2f(t0, t1);

    f32 t2 = 2.0 * (w * y - z * x);
    t2 = t2 > 1.0 ? 1.0 : t2;
    t2 = t2 < -1.0 ? -1.0 : t2;
    f32 pitch = asinf(t2);

    f32 t3 = 2.0 * (w * z + x * y);
    f32 t4 = 1.0 - 2.0 * (y * y + z * z);
    f32 yaw = atan2f(t3, t4);

    return sead::Vector3f(yaw, pitch, roll);
}

void logVector(const char* vectorName, sead::Vector3f vector) {
    hk::diag::logLine("%s: \nX: %f\nY: %f\nZ: %f", vectorName, vector.x, vector.y, vector.z);
}

void logQuat(const char* quatName, sead::Quatf& quat) {
    hk::diag::logLine("%s: \nX: %f\nY: %f\nZ: %f\nW: %f", quatName, quat.x, quat.y, quat.z, quat.w);
}

float vecMagnitude(sead::Vector3f const& input) {
    return (input.x * input.x + input.y * input.y + input.z * input.z);
}
#define DEG(rad) (rad * (180 / M_PI))  // converts Radians to Degrees
float quatAngle(sead::Quatf const& q1, sead::Quatf& q2) {
    float dot = (q1.x * q2.x) + (q1.y * q2.y) + (q1.z * q2.z) + (q1.w * q2.w);
    float dotAngle = sead::Mathf::min(abs(dot), 1.0f);

    return dotAngle > 1.0f - 0.000001f ? 0.0f : DEG(sead::Mathf::acos(dotAngle) * 2.0f);
}

bool isInCostumeList(const char* costumeName) {
    for (size_t i = 0; i < sizeof(costumeNames) / sizeof(costumeNames[0]); i++) {
        if (al::isEqualString(costumeNames[i], costumeName)) {
            return true;
        }
    }
    return false;
}

const char* tryGetPuppetCapName(PuppetInfo* info) {
    if (strcmp(info->costumeHead, "") != 0 && isInCostumeList(info->costumeHead)) {
        return info->costumeHead;
    } else {
        return "Mario";
    }
}

const char* tryGetPuppetBodyName(PuppetInfo* info) {
    if (strcmp(info->costumeBody, "") != 0 && isInCostumeList(info->costumeBody)) {
        return info->costumeBody;
    } else {
        return "Mario";
    }
}

const char* tryConvertName(const char* className) {
    for (size_t i = 0; i < hk::util::arraySize(classHackNames); i++) {
        if (al::isEqualString(classHackNames[i].className, className)) {
            return classHackNames[i].hackName;
        }
    }
    return className;
}

// Unity Classes
// Ultra-smooth exponential interpolation
float VisualUtils::SmoothMove(Transform moveTransform, Transform targetTransform, float timeDelta,
                              float closingSpeed, float maxAngularSpeed) {
    // Very responsive with minimal smoothing
    const float positionSmoothTime = 0.02f;
    const float rotationSmoothTime = 0.02f;

    float posLerpFactor = 1.0f - sead::Mathf::exp(-timeDelta / positionSmoothTime);
    al::lerpVec(moveTransform.position, *moveTransform.position, *targetTransform.position, posLerpFactor);

    if (moveTransform.rotation) {
        float rotLerpFactor = 1.0f - sead::Mathf::exp(-timeDelta / rotationSmoothTime);
        al::slerpQuat(moveTransform.rotation, *moveTransform.rotation, *targetTransform.rotation,
                      rotLerpFactor);
    }

    sead::Vector3f posDiff = *targetTransform.position - *moveTransform.position;
    return posDiff.length() / positionSmoothTime;
}

void killMainPlayer(al::LiveActor* actor) {
    PlayerActorHakoniwa* mainPlayer = (PlayerActorHakoniwa*)al::getPlayerActor(actor, 0);

    GameDataFunction::killPlayer(GameDataHolderWriter(actor));
    mainPlayer->startDemoPuppetable();
    al::setVelocityZero(mainPlayer);
    mainPlayer->mAnimator->endSubAnim();
    mainPlayer->mAnimator->startAnimDead();
}

void killMainPlayer(PlayerActorHakoniwa* mainPlayer) {
    GameDataFunction::killPlayer(GameDataHolderWriter(mainPlayer));
    mainPlayer->startDemoPuppetable();
    al::setVelocityZero(mainPlayer);
    mainPlayer->mAnimator->endSubAnim();
    mainPlayer->mAnimator->startAnimDead();
}

StageScene* getStageScene() {
    al::Sequence* curSequence = GameSystemFunction::getGameSystem()->mSequence;
    if (curSequence && al::isEqualString(curSequence->mName.cstr(), "HakoniwaSequence")) {
        auto gameSeq = (HakoniwaSequence*)curSequence;
        auto curScene = gameSeq->mCurrentScene;

        if (curScene && curScene->mIsAlive && al::isEqualString(curScene->mName.cstr(), "StageScene"))
            return (StageScene*)curScene;
    }
    return nullptr;
}