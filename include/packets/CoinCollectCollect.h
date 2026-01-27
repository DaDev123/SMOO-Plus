#pragma once

#include "Packet.h"
#include "Library/Placement/PlacementId.h"

struct PACKED CoinCollectCollect : Packet {
    CoinCollectCollect() : Packet() {
        this->mType = PacketType::COINCOLLECTCOLL;
        mPacketSize = sizeof(CoinCollectCollect) - sizeof(Packet);
    };
    const al::PlacementId* placeID;
    int worldID;
    sead::FixedSafeString<128> stage;
};