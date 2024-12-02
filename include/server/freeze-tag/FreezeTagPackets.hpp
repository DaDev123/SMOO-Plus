#pragma once

#include "packets/GameModeInf.h"

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

struct PACKED FreezeTagRoundStartPacket : GameModeInf<FreezeUpdateType> {
    FreezeTagRoundStartPacket() : GameModeInf() {
        setGameMode(GameMode::FREEZETAG);
        setUpdateType(FreezeUpdateType::ROUNDSTART);
        mPacketSize = sizeof(FreezeTagRoundStartPacket) - sizeof(Packet);
    };
    uint8_t    roundTime  = 10;
    const char padding[3] = "\0\0"; // to not break compatibility with old clients/servers that assume a size of 4 bytes minimum for GameModeInf packets
};

struct PACKED FreezeTagRoundCancelPacket : GameModeInf<FreezeUpdateType> {
    FreezeTagRoundCancelPacket() : GameModeInf() {
        setGameMode(GameMode::FREEZETAG);
        setUpdateType(FreezeUpdateType::ROUNDCANCEL);
        mPacketSize = sizeof(FreezeTagRoundCancelPacket) - sizeof(Packet);
    };
    const char padding[4]    = "\0\0\0"; // to not break compatibility with old clients/servers that assume a size of 4 bytes minimum for GameModeInf packets
    bool       onlyForLegacy = false;    // extra added byte only used by newer clients to ignore this packet conditionally
};
