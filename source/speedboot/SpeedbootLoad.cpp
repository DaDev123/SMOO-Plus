#include "speedboot/SpeedbootLoad.hpp"

#include "al/util.hpp"
#include "al/util/LayoutUtil.h"
#include "al/util/LiveActorUtil.h"
#include "al/util/MathUtil.h"

#include "sead/gfx/seadColor.h"
#include "sead/math/seadMathCalcCommon.h"
#include "sead/prim/seadSafeString.h"

#include "server/DeltaTime.hpp"

namespace speedboot {
    SpeedbootLoad::SpeedbootLoad(
        WorldResourceLoader* resourceLoader,
        const al::LayoutInitInfo& initInfo,
        float autoCloseAfter
    ) : al::LayoutActor("SpeedbootLoad"), worldResourceLoader(resourceLoader), mAutoCloseAfter(autoCloseAfter) {
        al::initLayoutActor(this, initInfo, "SpeedbootLoad", nullptr);
        initNerve(&nrvSpeedbootLoadAppear, 0);
        appear();
    }

    void SpeedbootLoad::exeAppear() {
        if (al::isFirstStep(this)) {
            al::startAction(this, "Appear", nullptr);
        }

        if (al::isActionEnd(this, nullptr)) {
            al::setNerve(this, &nrvSpeedbootLoadWait);
        }
    }

    void SpeedbootLoad::exeWait() {
        if (al::isActionEnd(this, nullptr)) {
            al::setNerve(this, &nrvSpeedbootLoadDecrease);
        }
    }

    void SpeedbootLoad::exeDecrease() {
        mTime += 0.016666f;

        mProgression = (
            mAutoCloseAfter <= 0.1f
            ? worldResourceLoader->calcLoadPercent() / 100.0f
            : mTime / mAutoCloseAfter
        );

        float rotation = cosf(mTime) * 3;

        // Debug stuff
        sead::WFormatFixedSafeString<0x40> string(u"Time: %.02f\nSine Value: %.02f", mTime, rotation);
        al::setPaneString(this, "TxtDebug", string.cstr(), 0);

        if (mProgression < 1.f) {
            // Target setup
            if (
                (mAutoCloseAfter <= 0.1f && mTime < 7.f)
                || (mAutoCloseAfter > 0.1f && mTime < (mAutoCloseAfter * 0.5f))
            ) {
                mOnlineLogoTransTarget   = { 0.f, 0.f };
                mOnlineLogoScaleTarget   = 1.f;
                mOnlineCreditScaleTarget = 1.f;

                mFreezeLogoTransXTarget = 1000.f;

                mFreezeBorderTarget = sead::Vector2f(700.f, 420.f);

                mFreezeBGTransXTarget = 0.f;
            } else {
                mOnlineLogoTransTarget   = { -520.f, 260.f };
                mOnlineLogoScaleTarget   = 0.3f;
                mOnlineCreditScaleTarget = 0.f;

                mFreezeLogoTransXTarget = 0.f;

                mFreezeBorderTarget = sead::Vector2f(640.f, 360.f);

                mFreezeBGTransXTarget = -1280.f;
            }

            // SMOO logo
            mOnlineLogoTrans.x   = al::lerpValue(mOnlineLogoTrans.x, mOnlineLogoTransTarget.x, 0.06f);
            mOnlineLogoTrans.y   = al::lerpValue(mOnlineLogoTrans.y, mOnlineLogoTransTarget.y, 0.06f);
            mOnlineLogoScale     = al::lerpValue(mOnlineLogoScale,   mOnlineLogoScaleTarget,   0.06f);
            mOnlineCreditScale   = al::lerpValue(mOnlineCreditScale, mOnlineCreditScaleTarget, 0.06f);
            al::setPaneLocalTrans(this, "PartOnlineLogo",      mOnlineLogoTrans);
            al::setPaneLocalScale(this, "PartOnlineLogo",      { mOnlineLogoScale,   mOnlineLogoScale   });
            al::setPaneLocalScale(this, "PicCraftyBossCredit", { mOnlineCreditScale, mOnlineCreditScale });

            // Freeze-Tag logo
            mFreezeLogoTransX = al::lerpValue(mFreezeLogoTransX, mFreezeLogoTransXTarget, 0.04f);
            al::setPaneLocalTrans(this, "FreezeLogoRoot", { mFreezeLogoTransX, 0.f });
            al::setPaneLocalRotate(this, "PicFreezeLogo", { 0.f, 0.f, rotation });

            // Freeze borders
            mFreezeBorder.x = al::lerpValue(mFreezeBorder.x, mFreezeBorderTarget.x, 0.02f);
            mFreezeBorder.y = al::lerpValue(mFreezeBorder.y, mFreezeBorderTarget.y, 0.02f);
            al::setPaneLocalTrans(this, "PicFreezeEdgeLeft",  { -mFreezeBorder.x, 0.f });
            al::setPaneLocalTrans(this, "PicFreezeEdgeRight", {  mFreezeBorder.x, 0.f });
            al::setPaneLocalTrans(this, "PicFreezeEdgeTop",   { 0.f,               mFreezeBorder.y });
            al::setPaneLocalTrans(this, "PicFreezeEdgeBot",   { 0.f,              -mFreezeBorder.y });

            // Freeze BG
            mFreezeBGTransX = al::lerpValue(mFreezeBGTransX, mFreezeBGTransXTarget, 0.04f);
            al::setPaneLocalTrans(this, "BackgroundsRoot", { mFreezeBGTransX, 0.f });
        }

        if (mProgression >= 1.f) {
            al::setNerve(this, &nrvSpeedbootLoadEnd);
        }
    }

    void SpeedbootLoad::exeEnd() {
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
    }
}
