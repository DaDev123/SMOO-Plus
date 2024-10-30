#pragma once

#include "al/camera/CameraTicket.h"

#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/speedrun/SpeedrunIcon.h"
#include "server/speedrun/SpeedrunInfo.hpp"

class SpeedrunMode : public GameModeBase {
    public:
        SpeedrunMode(const char* name);

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

        inline bool isPlayerSeeking() const { return mInfo->isPlayerSeeking(); }
        inline bool isPlayerHiding()  const { return mInfo->isPlayerHiding();  }

        float getInvulnTime() const { return mInvulnTime; }

        void enableGravityMode() { mInfo->mIsUseGravity = true; }
        void disableGravityMode() { mInfo->mIsUseGravity = false; }
        bool isUseGravity() const { return mInfo->mIsUseGravity; }
        void onBorderPullBackFirstStep(al::LiveActor* actor) override;

        bool hasCustomCamera() const override { return true; }
        void createCustomCameraTicket(al::CameraDirector* director) override;

        bool hasMarioCollision() override { return SpeedrunInfo::mHasMarioCollision; }
        bool hasMarioBounce()    override { return SpeedrunInfo::mHasMarioBounce;    }
        bool hasCappyCollision() override { return SpeedrunInfo::mHasCappyCollision; }
        bool hasCappyBounce()    override { return SpeedrunInfo::mHasCappyBounce;    }

    private:
        float             mInvulnTime = 0.0f;
        GameModeTimer*    mModeTimer  = nullptr;
        SpeedrunIcon*  mModeLayout = nullptr;
        SpeedrunInfo*  mInfo       = nullptr;
        al::CameraTicket* mTicket     = nullptr;

        void updateTagState(bool isSeeking);
};
