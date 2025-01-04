#include "speedboot/SpeedbootLoad.hpp"
#include "DeltaTime.hpp"
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
#include "prim/seadSafeString.h"

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
    mProgression = worldResourceLoader->calcLoadPercent() / 100.0f;
    sead::Color4u8 color = { 255, 255, 0, 255 };

    color.r = 255 - sead::MathCalcCommon<int>::ceil(mProgression * 255);
    color.g = sead::MathCalcCommon<int>::ceil(mProgression * 255);

    mRotTime += 0.03f;
    float rotation = cosf(mRotTime) * 5;

    sead::WFormatFixedSafeString<0x40> string(u"Display Time: %.02f\nSin Value: %.02f", mRotTime, rotation);

    al::setPaneString(this, "TxtDebug", string.cstr(), 0);

    if (mProgression < 1.0) {
        // al::startFreezeAction(this, "Decrease", 60.f * (1.0f - actual), "State");
        al::setPaneLocalScale(this, "PicBar", { mProgression, 2.f });
        al::setPaneVtxColor(this, "PicBar", color);

        al::setPaneLocalScale(this, "PicBarFill", { 30.f, 1.f });
        al::setPaneVtxColor(this, "PicBarFill", color);

        al::setPaneLocalTrans(this, "PicPointer", { (mProgression * 1280.f) - 640.f, -260.f });
        al::setPaneLocalRotate(this, "PicPointer", { 0.f, 0.f, rotation });

        al::setPaneLocalRotate(this, "PicBG", { 0.f, 0.f, mRotTime * -3.f });
    }

    if (mProgression > 1.0f) {
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