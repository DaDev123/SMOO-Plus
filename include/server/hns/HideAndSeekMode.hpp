#pragma once

#include <math.h>
#include <basis/seadTypes.h>

#include "al/camera/CameraTicket.h"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/hns/HideAndSeekIcon.h"
#include "server/hns/HideAndSeekInfo.hpp"

class HideAndSeekMode : public GameModeBase {
    public:
        HideAndSeekMode(const char* name);

        void init(GameModeInitInfo const& info) override;

        void begin() override;
        void update() override;
        void end() override;

        void pause() override;
        void unpause() override;

        bool showNameTag(PuppetInfo* other) override;

        void debugMenuControls(sead::TextWriter* gTextWriter) override;

        bool isUseNormalUI() const override { return false; }

        void processPacket(Packet* packet) override;
        Packet* createPacket() override;

        bool isPlayerIt() const { return mInfo->mIsPlayerIt; }

        float getInvulnTime() const { return mInvulnTime; }

        void setPlayerTagState(bool state) { mInfo->mIsPlayerIt = state; }

        void enableGravityMode() { mInfo->mIsUseGravity = true; }
        void disableGravityMode() { mInfo->mIsUseGravity = false; }
        bool isUseGravity() const { return mInfo->mIsUseGravity; }
        void onBorderPullBackFirstStep(al::LiveActor* actor) override;

        bool hasCustomCamera() const override { return true; }
        void createCustomCameraTicket(al::CameraDirector* director) override;

    private:
        float             mInvulnTime = 0.0f;
        GameModeTimer*    mModeTimer  = nullptr;
        HideAndSeekIcon*  mModeLayout = nullptr;
        HideAndSeekInfo*  mInfo       = nullptr;
        al::CameraTicket* mTicket     = nullptr;

        void updateTagState(bool isSeeking);
};
