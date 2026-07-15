#pragma once

#include "packets/Packet.h"

struct __attribute__((packed)) ShineCollect : Packet {
    ShineCollect() : Packet() {
        this->mType = PacketType::SHINECOLL;
        mPacketSize = sizeof(ShineCollect) - sizeof(Packet);
    };
    int shineId = -1;
    u8 isGrand = false;  // fake bool
};