#pragma once

#include "packets/GameModeInf.h"

struct HnSUpdateTypes {
    enum Type : u8 {
        TIME  = 1 << 0,
        STATE = 1 << 1
    };
};
typedef HnSUpdateTypes::Type HnSUpdateType;

struct PACKED HideAndSeekPacket : GameModeInf<HnSUpdateType> {
    HideAndSeekPacket() : GameModeInf() {
        setGameMode(GameMode::HIDEANDSEEK);
        mPacketSize = sizeof(HideAndSeekPacket) - sizeof(Packet);
    };
    bool1 isIt = false;
    u8    seconds;
    u16   minutes;
};
