#pragma once

#include "packets/GameModeInf.h"

struct HnSUpdateTypes {
    enum Type : u8 {
        TIME  = 1 << 0,
        STATE = 1 << 1
    };
};
typedef HnSUpdateTypes::Type HnSUpdateType;

struct PACKED SpeedrunPacket : GameModeInf<HnSUpdateType> {
    SpeedrunPacket() : GameModeInf() {
        setGameMode(GameMode::SPEEDRUN);
        mPacketSize = sizeof(SpeedrunPacket) - sizeof(Packet);
    };
    bool1 isIt = false;
    u8    seconds;
    u16   minutes;
};
