#pragma once

#include <math.h>
#include <basis/seadTypes.h>

#include "al/camera/CameraTicket.h"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeConfigMenu.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/inf/InfectionConfigMenu.hpp"

#include "packets/Packet.h"

struct InfectionInfo : GameModeInfoBase {
    InfectionInfo() { mMode = GameMode::INFECTION; }
    bool mIsPlayerIt = false;
    bool mIsUseGravity = false;
    bool mIsUseGravityCam = false;
    bool mIsUseSlipperyGround = true;
    GameTime mHidingTime;
};

class InfectionMode : public GameModeBase {
    public:
        InfectionMode(const char* name);

        void init(GameModeInitInfo const& info) override;

        void begin() override;
        void update() override;
        void end() override;
    
        void pause() override;
        void unpause() override;

        bool isUseNormalUI() const override { return false; }

        void processPacket(Packet* packet) override;
        Packet* createPacket() override;

        bool isPlayerIt() const { return mInfo->mIsPlayerIt; }

        float getInvulnTime() const { return mInvulnTime; }

        void setPlayerTagState(bool state) { mInfo->mIsPlayerIt = state; }

        void enableGravityMode() {mInfo->mIsUseGravity = true;}
        void disableGravityMode() { mInfo->mIsUseGravity = false; }
        bool isUseGravity() const { return mInfo->mIsUseGravity; }

        void setCameraTicket(al::CameraTicket* ticket) { mTicket = ticket; }

    private:
        float mInvulnTime = 0.0f;
        GameModeTimer* mModeTimer = nullptr;
        InfectionIcon *mModeLayout = nullptr;
        InfectionInfo* mInfo = nullptr;
        al::CameraTicket *mTicket = nullptr;

};