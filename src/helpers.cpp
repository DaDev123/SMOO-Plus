#include "helpers.hpp"

#include "hk/util/Algorithm.h"

#include "al/Library/Math/MathUtil.h"

bool isInCostumeList(const char* costumeName) {
    for (size_t i = 0; i < sizeof(costumeNames) / sizeof(costumeNames[0]); i++) {
        if (al::isEqualString(costumeNames[i], costumeName)) {
            return true;
        }
    }
    return false;
}

const char* tryGetPuppetCapName(PuppetInfo* info) {
    if (!info->costumeHead.isEmpty() && isInCostumeList(info->costumeHead.cstr())) {
        return info->costumeHead.cstr();
    } else {
        return "Mario";
    }
}

const char* tryGetPuppetBodyName(PuppetInfo* info) {
    if (!info->costumeBody.isEmpty() && isInCostumeList(info->costumeBody.cstr())) {
        return info->costumeBody.cstr();
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
