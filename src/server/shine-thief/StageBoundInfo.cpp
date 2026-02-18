#include "server/shine-thief/StageBoundInfo.h"

#include "al/Library/Collision/CollisionPartsKeeperUtil.h"

#include "Library/LiveActor/LiveActor.h"
#include "logger.hpp"

void StageBoundInfo::init(al::LiveActor* p1, float checkRange, float checkInterval) {
    if (isInit)
        return;
    float checkHeight = 50000.f;
    sead::Vector3f ray = {0.f, -checkHeight * 2.f, 0.f};

    for (float xLoop = -checkRange; xLoop < checkRange; xLoop += checkInterval) {
        for (float zLoop = -checkRange; zLoop < checkRange; zLoop += checkInterval) {
            sead::Vector3f pos = sead::Vector3f::zero;
            bool isFind = alCollisionUtil::getFirstPolyOnArrow(p1, &pos, nullptr, {xLoop, checkHeight, zLoop}, ray, nullptr, nullptr);
            if (isFind) {
                if (pos.x < mMin.x)
                    mMin.x = pos.x;
                if (pos.z < mMin.z)
                    mMin.z = pos.z;

                if (pos.x > mMax.x)
                    mMax.x = pos.x;
                if (pos.z > mMax.z)
                    mMax.z = pos.z;

                if (mPeakHeight < pos.y)
                    mPeakHeight = pos.y;
            }
        }
    }

    mPeakHeight += 250.f;
    isInit = true;
    Logger::log("Inited Stage Bounds:\n Max X: %f, Max Z: %f, Min X: %f, Min Z: %f, Max Y: %f\n", mMax.x, mMax.z, mMin.x, mMin.z, mPeakHeight);
}