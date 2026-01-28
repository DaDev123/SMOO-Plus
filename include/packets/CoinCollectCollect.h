#pragma once

#include "Packet.h"

struct PACKED CoinCollectCollect : Packet {
    CoinCollectCollect() : Packet() {
        this->mType = PacketType::COINCOLLECTCOLL;
        mPacketSize = sizeof(CoinCollectCollect) - sizeof(Packet);
    };
    char placeID[16] = {};
    int worldID;
    char stage[64] = {};
};