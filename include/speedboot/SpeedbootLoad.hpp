#include "al/layout/LayoutActor.h"
#include "al/layout/LayoutInitInfo.h"
#include "al/util/NerveUtil.h"

class WorldResourceLoader;

class SpeedbootLoad : public al::LayoutActor {
public:
    SpeedbootLoad(WorldResourceLoader* resourceLoader, const al::LayoutInitInfo& initInfo);

    void exeAppear();
    void exeWait();
    void exeDecrease();
    void exeEnd();

    float mProgression = 0.f;
    float mRotTime = 0.f;

private:
    WorldResourceLoader* worldResourceLoader;
};

namespace {
NERVE_HEADER(SpeedbootLoad, Appear)
NERVE_HEADER(SpeedbootLoad, Wait)
NERVE_HEADER(SpeedbootLoad, Decrease)
NERVE_HEADER(SpeedbootLoad, End)
}