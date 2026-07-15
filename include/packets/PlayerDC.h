#pragma once

#include "Packet.h"

struct __attribute__((packed)) PlayerDC : Packet {
    PlayerDC() : Packet() {
        this->mType = PacketType::PLAYERDC;
        mPacketSize = sizeof(PlayerDC) - sizeof(Packet);
    };
};