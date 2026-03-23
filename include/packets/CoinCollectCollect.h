#pragma once

#include "Packet.h"

struct PACKED CoinCollectCollect : Packet {
    CoinCollectCollect() : Packet() {
        this->mType = PacketType::COINCOLLECTCOLL;
        mPacketSize = sizeof(CoinCollectCollect) - sizeof(Packet);
    };
    char placeID[64] = {};
    int worldID = 0;
    char stage[64] = {};
};
