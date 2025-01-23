#pragma once

#include "Packet.h"
#include "sead/basis/seadTypes.h"

struct PACKED FreezeInf : Packet {
    FreezeInf() : Packet() { this->mType = PacketType::TAGINF; mPacketSize = sizeof(FreezeInf) - sizeof(Packet);};
    TagUpdateType updateType;
    bool1 isRunner = false;
    bool1 isFreeze = false;
    uint16_t score;
};