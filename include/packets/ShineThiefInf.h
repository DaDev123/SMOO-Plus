#pragma once

#include "sead/basis/seadTypes.h"

#include "math/seadVectorFwd.h"
#include "Packet.h"

enum class ShineThiefUpdateType : u8 {
    PLAYER = 1 << 0,
    ROUNDSTART = 1 << 1,
    ROUNDCANCEL = 1 << 2,
    FALLOFF = 1 << 3,
};

enum class ShineThiefPostProcessingType : u8 { PPDISABLED = 0, PPENDGAMELOSE = 2, PPENDGAMEWIN = 3 };

struct PACKED ShineThiefInf : Packet {
    ShineThiefInf() : Packet() {
        this->mType = PacketType::TAGINF;
        mPacketSize = sizeof(ShineThiefInf) - sizeof(Packet);
    };
    ShineThiefUpdateType updateType;
    uint8_t team;
    bool isHolder;
    bool isCaught;
    uint16_t score;
    uint16_t padding;
    sead::Vector3f shinePos;
};

struct PACKED ShineThiefInfRoundPacket : Packet {
    ShineThiefInfRoundPacket() : Packet() {
        this->mType = PacketType::TAGINF;
        mPacketSize = sizeof(ShineThiefInfRoundPacket) - sizeof(Packet);
    };
    ShineThiefUpdateType updateType;
    uint8_t roundTime;
    char padding[3];
    sead::Vector3f shinePos;
    sead::Vector3f hostStartPos;
};