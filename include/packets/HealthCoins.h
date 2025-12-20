#pragma once

#include "Packet.h"

struct PACKED HealthCoins : Packet {
    InitPacket() : Packet() {
        this->mType = PacketType::HEALTHCOINS;
        mPacketSize = sizeof(HealthCoins) - sizeof(Packet);
    };
    u8 health = 0;
    int coins = 0;
};