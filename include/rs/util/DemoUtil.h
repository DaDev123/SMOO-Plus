#pragma once

#include "al/scene/Scene.h"
#include "game/Interfaces/IUseDemoSkip.h"
#include "al/LiveActor/LiveActor.h"

namespace rs {
    void requestValidateDemoSkip(IUseDemoSkip *,al::LiveActor const*);
    void getDemoSkipRequester(al::Scene const*);
    bool isDemoEnableSkip(al::Scene const*);
    bool isFirstDemo(al::Scene const*);
    bool isEnableSkipDemo(al::Scene const*);
    void skipDemo(al::Scene const*);
    void updateOnlyDemoGraphics(al::Scene const*);
}