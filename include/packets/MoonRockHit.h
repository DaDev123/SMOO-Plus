#pragma once

#include "Packet.h"

struct PACKED MoonRockHit : Packet {
    MoonRockHit() : Packet() {
        this->mType = PacketType::MOONROCKHIT;
        mPacketSize = sizeof(MoonRockHit) - sizeof(Packet);
    };
    int worldId = 0;
};