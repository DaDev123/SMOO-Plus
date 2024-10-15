#pragma once

#include "types.h"
#include "packets/GameModeInf.h"
#include "server/gamemode/GameMode.hpp"

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
