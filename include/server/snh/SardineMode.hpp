#pragma once

#include <math.h>
#include "al/Library/Camera/CameraTicket.h"
#include "layouts/SardineIcon.h"
#include "server/gamemode/GameModeBase.hpp"

#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"

struct SardineInfo : GameModeInfoBase {
    SardineInfo() { mMode = GameMode::SARDINE; }
    bool mIsIt = false;
    bool mIsUseGravity = false;
    bool mIsUseGravityCam = false;

    bool mIsTether = false;
    bool mIsTetherSnap = false;

    GameTime mHidingTime;
};

class SardineMode : public GameModeBase {
public:
    SardineMode(const char* name);

    void init(GameModeInitInfo const& info) override;

    virtual void begin() override;
    virtual void update() override;
    virtual void end() override;

    bool isUseNormalUI() const override { return false; }

    bool isPlayerIt() const { return mInfo->mIsIt; };

    void setPlayerTagState(bool state) { mInfo->mIsIt = state; }

    void enableGravityMode() { mInfo->mIsUseGravity = true; }
    void disableGravityMode() { mInfo->mIsUseGravity = false; }
    bool isUseGravity() const { return mInfo->mIsUseGravity; }

    void setCameraTicket(al::CameraTicket* ticket) { mTicket = ticket; }

private:
    GameModeTimer* mModeTimer = nullptr;
    SardineIcon* mModeLayout = nullptr;
    SardineInfo* mInfo = nullptr;
    al::CameraTicket* mTicket = nullptr;

    float pullDistanceMax = 2250.f;
    float pullDistanceMin = 1000.f;
    float pullPowerRate = 75.f;
};