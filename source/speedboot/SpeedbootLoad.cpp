#include "speedboot/SpeedbootLoad.hpp"
#include "al/layout/LayoutActor.h"
#include "al/util.hpp"
#include "al/util/LayoutUtil.h"
#include "al/util/LiveActorUtil.h"
#include "al/util/MathUtil.h"
#include "al/util/NerveUtil.h"
#include "game/WorldList/WorldResourceLoader.h"
#include "gfx/seadColor.h"
#include "logger.hpp"
#include "math/seadMathCalcCommon.h"
#include "math/seadVector.h"
#include "prim/seadSafeString.h"
#include "server/DeltaTime.hpp"

SpeedbootLoad::SpeedbootLoad(WorldResourceLoader* resourceLoader, const al::LayoutInitInfo& initInfo)
    : al::LayoutActor("SpeedbootLoad")
    , worldResourceLoader(resourceLoader)
{
    al::initLayoutActor(this, initInfo, "SpeedbootLoad", nullptr);
    initNerve(&nrvSpeedbootLoadAppear, 0);
    // al::setPaneLocalScale(this, "All", { 5.0f, 5.0f });
    appear();
}

void SpeedbootLoad::exeAppear()
{
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
    }

    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &nrvSpeedbootLoadWait);
    }
}

void SpeedbootLoad::exeWait()
{
    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &nrvSpeedbootLoadDecrease);
    }
}

void SpeedbootLoad::exeDecrease()
{
    mTime += 0.016666f;

    mProgression = worldResourceLoader->calcLoadPercent() / 100.0f;

    float rotation = cosf(mTime) * 3;

    // Debug stuff
    sead::WFormatFixedSafeString<0x40> string(u"Time: %.02f\nSine Value: %.02f", mTime, rotation);
    al::setPaneString(this, "TxtDebug", string.cstr(), 0);

    if (mProgression < 1.f) {

        // Target setup
        if (mTime < 7.f) {
            mOnlineLogoScaleTarget = 1.f;
            mOnlineLogoTransTarget = { 0.f, 0.f };

            mFreezeLogoTransXTarget = 1000.f;

            mFreezeBorderTarget = sead::Vector2f(700.f, 420.f);

            mFreezeBGTransXTarget = 0.f;
        } else {
            mOnlineLogoScaleTarget = 0.3f;
            mOnlineLogoTransTarget = { -520.f, 260.f };

            mFreezeLogoTransXTarget = 0.f;

            mFreezeBorderTarget = sead::Vector2f(640.f, 360.f);

            mFreezeBGTransXTarget = -1280.f;
        }

        // Online logo part //

        mOnlineLogoScale = al::lerpValue(mOnlineLogoScale, mOnlineLogoScaleTarget, 0.04f);
        mOnlineLogoTrans.x = al::lerpValue(mOnlineLogoTrans.x, mOnlineLogoTransTarget.x, 0.04f);
        mOnlineLogoTrans.y = al::lerpValue(mOnlineLogoTrans.y, mOnlineLogoTransTarget.y, 0.04f);

        al::setPaneLocalScale(this, "PartOnlineLogo", { mOnlineLogoScale, mOnlineLogoScale });
        al::setPaneLocalTrans(this, "PartOnlineLogo", mOnlineLogoTrans);

        // Freeze logo part //

        mFreezeLogoTransX = al::lerpValue(mFreezeLogoTransX, mFreezeLogoTransXTarget, 0.02f);

        al::setPaneLocalTrans(this, "FreezeLogoRoot", { mFreezeBGTransX, 0.f });
        al::setPaneLocalRotate(this, "PicFreezeLogo", { 0.f, 0.f, 0.f });

        // Freeze borders //

        mFreezeBorder.x = al::lerpValue(mFreezeBorder.x, mFreezeBorderTarget.x, 0.01f);
        mFreezeBorder.y = al::lerpValue(mFreezeBorder.y, mFreezeBorderTarget.y, 0.01f);

        al::setPaneLocalTrans(this, "PicFreezeEdgeLeft", { -mFreezeBorder.x, 0.f });
        al::setPaneLocalTrans(this, "PicFreezeEdgeRight", { mFreezeBorder.x, 0.f });
        al::setPaneLocalTrans(this, "PicFreezeEdgeTop", { 0.f, mFreezeBorder.y });
        al::setPaneLocalTrans(this, "PicFreezeEdgeBot", { 0.f, -mFreezeBorder.y });

        // Freeze BG //
        mFreezeBGTransX = al::lerpValue(mFreezeBGTransX, mFreezeBGTransXTarget, 0.02f);

        al::setPaneLocalTrans(this, "BackgroundsRoot", { mFreezeBGTransX, 0.f });
    }

    if (mProgression > 1.f) {
        al::setNerve(this, &nrvSpeedbootLoadEnd);
    }
}

void SpeedbootLoad::exeEnd()
{
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
    }

    if (al::isActionEnd(this, nullptr)) {
        kill();
    }
}

namespace {
NERVE_IMPL(SpeedbootLoad, Appear)
NERVE_IMPL(SpeedbootLoad, Wait)
NERVE_IMPL(SpeedbootLoad, Decrease)
NERVE_IMPL(SpeedbootLoad, End)
} // namespace