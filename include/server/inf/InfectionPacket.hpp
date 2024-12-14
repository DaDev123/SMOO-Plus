#pragma once

#include "packets/GameModeInf.h"

struct InFUpdateTypes {
    enum Type : u8 {
        TIME  = 1 << 0,
        STATE = 1 << 1
    };
};
typedef InFUpdateTypes::Type InFUpdateType;

struct PACKED InfectionPacket : GameModeInf<InFUpdateType> {
    InfectionPacket() : GameModeInf() {
        setGameMode(GameMode::INFECTION);
        mPacketSize = sizeof(InfectionPacket) - sizeof(Packet);
    };
    bool1 isIt = false;
    u8    seconds;
    u16   minutes;
};
