#include "al/layout/LayoutActor.h"
#include "al/layout/LayoutInitInfo.h"
#include "al/util/NerveUtil.h"
#include "math/seadVector.h"

class WorldResourceLoader;

class SpeedbootLoad : public al::LayoutActor {
public:
    SpeedbootLoad(WorldResourceLoader* resourceLoader, const al::LayoutInitInfo& initInfo);

    void exeAppear();
    void exeWait();
    void exeDecrease();
    void exeEnd();

    float mTime = 0.f;
    float mProgression = 0.f;
    float mRotTime = 0.f;

    // Online logo part
    float mOnlineLogoScale = 0.f;
    float mOnlineLogoScaleTarget = 0.f;
    sead::Vector2f mOnlineLogoTrans = sead::Vector2f::zero;
    sead::Vector2f mOnlineLogoTransTarget = sead::Vector2f::zero;

    // Freze tag logo root
    float mFreezeLogoTransX = 1000.f;
    float mFreezeLogoTransXTarget = 1000.f;

    // Borders
    sead::Vector2f mFreezeBorder = {700.f, 420.f};
    sead::Vector2f mFreezeBorderTarget = {700.f, 420.f};

    // Backgrounds
    float mFreezeBGTransX = 0.f;
    float mFreezeBGTransXTarget = 0.f;

private:
    WorldResourceLoader* worldResourceLoader;
};

namespace {
NERVE_HEADER(SpeedbootLoad, Appear)
NERVE_HEADER(SpeedbootLoad, Wait)
NERVE_HEADER(SpeedbootLoad, Decrease)
NERVE_HEADER(SpeedbootLoad, End)
}