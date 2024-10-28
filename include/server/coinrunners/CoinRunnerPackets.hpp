#pragma once

#include "packets/GameModeInf.h"

struct CoinUpdateTypes {
    enum Type : u8 { // Type of packets to send between players
        PLAYER      = 1 << 0,
        ROUNDSTART  = 1 << 1,
        ROUNDCANCEL = 1 << 2,
        FALLOFF     = 1 << 3,
    };
};
typedef CoinUpdateTypes::Type CoinUpdateType;

struct PACKED CoinRunnerPacket : GameModeInf<CoinUpdateType> {
    CoinRunnerPacket() : GameModeInf() {
        setGameMode(GameMode::COINRUNNER);
        mPacketSize = sizeof(CoinRunnerPacket) - sizeof(Packet);
    };
    bool     isRunner = false;
    bool     isFreeze = false;
    uint16_t score    = 0;
};

struct PACKED CoinRunnerRoundStartPacket : GameModeInf<CoinUpdateType> {
    CoinRunnerRoundStartPacket() : GameModeInf() {
        setGameMode(GameMode::COINRUNNER);
        setUpdateType(CoinUpdateType::ROUNDSTART);
        mPacketSize = sizeof(CoinRunnerRoundStartPacket) - sizeof(Packet);
    };
    uint8_t    roundTime  = 10;
    const char padding[3] = "\0\0"; // to not break compatibility with old clients/servers that assume a size of 4 bytes minimum for GameModeInf packets
};

struct PACKED CoinRunnerRoundCancelPacket : GameModeInf<CoinUpdateType> {
    CoinRunnerRoundCancelPacket() : GameModeInf() {
        setGameMode(GameMode::COINRUNNER);
        setUpdateType(CoinUpdateType::ROUNDCANCEL);
        mPacketSize = sizeof(CoinRunnerRoundCancelPacket) - sizeof(Packet);
    };
    const char padding[4]    = "\0\0\0"; // to not break compatibility with old clients/servers that assume a size of 4 bytes minimum for GameModeInf packets
    bool       onlyForLegacy = false;    // extra added byte only used by newer clients to ignore this packet conditionally
};
