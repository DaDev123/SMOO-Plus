#pragma once

#include "packets/Packet.h"

struct __attribute__((packed)) PlayerDC : Packet {
    PlayerDC() : Packet() {
        this->mType = PacketType::PLAYERDC;
        mPacketSize = sizeof(PlayerDC) - sizeof(Packet);
    };
};