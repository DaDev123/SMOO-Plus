#pragma once

#include "Packet.h"
#include "server/Client.hpp"

template <typename UpdateType>
struct PACKED GameModeInf : Packet {
    GameModeInf() : Packet() {
        this->mType = PacketType::GAMEMODEINF;
        mPacketSize = sizeof(GameModeInf) - sizeof(Packet);
    };
    u8 mModeAndType = 0;

    GameMode gameMode() {
        // Extract the game mode from the upper 4 bits and adjust for NONE = -1
        u8 rawMode = mModeAndType >> 4;
        GameMode mode = (GameMode)(rawMode - 1);  // Subtract 1 to map: 0->-1, 1->0, 2->1, 3->2
        
        // Validate it's a known game mode
        if (mode < NONE || mode > FREEZETAG) {
            return GameMode::NONE;
        }
        
        return mode;
    }

    UpdateType updateType() {
        return static_cast<UpdateType>(mModeAndType & 0x0f);
    }

    void setGameMode(GameMode mode) {
        // Add 1 to map: -1->0, 0->1, 1->2, 2->3, then shift to upper 4 bits
        u8 adjustedMode = (u8)(mode + 1);
        mModeAndType = (adjustedMode << 4) | (mModeAndType & 0x0f);
    }

    void setUpdateType(UpdateType type) {
        mModeAndType = (mModeAndType & 0xf0) | (type & 0x0f);
    }
};

struct PACKED DisabledGameModeInf : GameModeInf<u8> {
    DisabledGameModeInf(nn::account::Uid userID) : GameModeInf() {
        setGameMode(GameMode::NONE);
        setUpdateType(3); // so that legacy Hide&Seek and Sardines clients will parse isIt = false
        mUserID     = userID;
        mPacketSize = sizeof(DisabledGameModeInf) - sizeof(Packet);
    };
    bool isIt    = false;
    u8   seconds = 0;
    u16  minutes = 0;
};