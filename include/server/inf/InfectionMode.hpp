#pragma once

#include <math.h>
#include "al/camera/CameraTicket.h"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeConfigMenu.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/inf/InfectionConfigMenu.hpp"

#include "packets/Packet.h"

struct InfectionInfo : GameModeInfoBase {
    InfectionInfo() { mMode = GameMode::Infection; }
    bool mIsPlayerIt = false;
    bool mIsUseGravity = false;
    bool mIsUseGravityCam = false;
    GameTime mHidingTime;
};

enum InfectionTagUpdateType : u8 {
    InfectionTIME                 = 1 << 0,
    InfectionSTATE                = 1 << 1
};
struct PACKED InfectionPacket : Packet {
    InfectionPacket() : Packet() { this->mType = PacketType::GAMEMODEINF; mPacketSize = sizeof(InfectionPacket) - sizeof(Packet);};
    InfectionTagUpdateType updateType;
    bool1 isIt = false;
    u8 seconds;
    u16 minutes;
};

class InfectionMode : public GameModeBase {
    public:
        InfectionMode(const char* name);

        void init(GameModeInitInfo const& info) override;

        virtual void begin() override;
        virtual void update() override;
        virtual void end() override;

        bool isUseNormalUI() const override { return false; }       

        void processPacket(Packet* packet) override;
        Packet* createPacket() override;

        void pause() override;
        void unpause() override;

        bool isPlayerIt() const { return mInfo->mIsPlayerIt; };

        void setPlayerTagState(bool state) { mInfo->mIsPlayerIt = state; }

        void enableGravityMode() {mInfo->mIsUseGravity = true;}
        void disableGravityMode() { mInfo->mIsUseGravity = false; }
        bool isUseGravity() const { return mInfo->mIsUseGravity; }

        void setCameraTicket(al::CameraTicket *ticket) {mTicket = ticket;}

    private:
        float mInvulnTime = 0.0f;
        GameModeTimer* mModeTimer = nullptr;
        InfectionIcon *mModeLayout = nullptr;
        InfectionInfo* mInfo = nullptr;
        al::CameraTicket *mTicket = nullptr;

};