#pragma once

#include "Packet.h"

struct __attribute__((packed)) CoinCollectCollect : Packet {
    CoinCollectCollect() : Packet() {
        this->mType = PacketType::COINCOLLECTCOLL;
        mPacketSize = sizeof(CoinCollectCollect) - sizeof(Packet);
    };
    char placeID[0x40] = {};
    int worldID = 0;
    char stage[0x40] = {};
};
