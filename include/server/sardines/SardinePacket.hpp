#pragma once

#include "packets/GameModeInf.h"

struct SardineUpdateTypes {
    enum Type : u8 {
        TIME  = 1 << 0,
        STATE = 1 << 1
    };
};
typedef SardineUpdateTypes::Type SardineUpdateType;

struct PACKED SardinePacket : GameModeInf<SardineUpdateType> {
    SardinePacket() : GameModeInf() {
        setGameMode(GameMode::SARDINE);
        mPacketSize = sizeof(SardinePacket) - sizeof(Packet);
    };
    bool1 isIt = false;
    u8    seconds;
    u16   minutes;
};
