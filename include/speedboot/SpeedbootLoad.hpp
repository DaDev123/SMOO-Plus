#pragma once

#include "al/layout/LayoutActor.h"
#include "al/layout/LayoutInitInfo.h"

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

    // Dot animation (used for loading and connecting)
    float mDotTimer = 0.0f;
    int mLoadingDotState = 0;
    int mConnectingDotState = 0;

    // Connection state
    bool mHasConnected = false;
    float mConnectionDisplayTimer = 0.0f;

    // Movement after connection
    float mMoveTimer = 0.0f;
    bool mHasStartedMove = false;
};
