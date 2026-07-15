#pragma once

#include "packets/Packet.h"

struct __attribute__((packed)) MoonRockHit : Packet {
    MoonRockHit() : Packet() {
        this->mType = PacketType::MOONROCKHIT;
        mPacketSize = sizeof(MoonRockHit) - sizeof(Packet);
    };
    int worldId = 0;
};