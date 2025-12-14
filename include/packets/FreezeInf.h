#pragma once

#include "Packet.h"
#include "sead/basis/seadTypes.h"

enum FreezeUpdateType : u8 { // Type of packets to send between players
    PLAYER                 = 1 << 0,
    ROUNDSTART             = 1 << 1,
    ROUNDCANCEL            = 1 << 2,
    FALLOFF                = 1 << 3
};

enum FreezePostProcessingType : u8 { // Snapshot mode post processing state
    PPDISABLED = 0,
    PPFROZEN = 1,
    PPENDGAMELOSE = 2,
    PPENDGAMEWIN = 3
};

struct PACKED FreezeInf : Packet {
    FreezeInf() : Packet() { this->mType = PacketType::TAGINF; mPacketSize = sizeof(FreezeInf) - sizeof(Packet);};
    FreezeUpdateType updateType;
    bool isRunner = false;
    bool isFreeze = false;
    uint16_t score = 0;
};

struct PACKED FreezeInfRoundPacket : Packet {
    FreezeInfRoundPacket() : Packet() { 
        this->mType = PacketType::TAGINF; 
        mPacketSize = sizeof(FreezeInfRoundPacket) - sizeof(Packet); // Fixed size calculation
    };
    FreezeUpdateType updateType;
    uint8_t roundTime = 10;
    const char padding[3] = "\0\0";
};