#pragma once

#include "types.h"
#include "packets/GameModeInf.h"
#include "server/gamemode/GameMode.hpp"

struct FreezeUpdateTypes {
    enum Type : u8 { // Type of packets to send between players
        PLAYER      = 1 << 0,
        ROUNDSTART  = 1 << 1,
        ROUNDCANCEL = 1 << 2,
        FALLOFF     = 1 << 3,
    };
};
typedef FreezeUpdateTypes::Type FreezeUpdateType;

struct PACKED FreezeTagPacket : GameModeInf<FreezeUpdateType> {
    FreezeTagPacket() : GameModeInf() {
        setGameMode(GameMode::FREEZETAG);
        mPacketSize = sizeof(FreezeTagPacket) - sizeof(Packet);
    };
    bool     isRunner = false;
    bool     isFreeze = false;
    uint16_t score    = 0;
};

struct PACKED FreezeTagRoundPacket : GameModeInf<FreezeUpdateType> {
    FreezeTagRoundPacket() : GameModeInf() {
        setGameMode(GameMode::FREEZETAG);
        mPacketSize = sizeof(FreezeTagRoundPacket) - sizeof(Packet);
    };
    uint8_t    roundTime  = 10;
    const char padding[3] = "\0\0"; // to not break compatibility with old clients/servers that assume a size of 4 bytes minimum for GameModeInf packets
};
