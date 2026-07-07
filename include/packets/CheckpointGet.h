#pragma once

#include "Packet.h"

struct PACKED CheckpointGet : Packet {
    CheckpointGet() : Packet() {
        this->mType = PacketType::CHECKPOINTGET;
        mPacketSize = sizeof(CheckpointGet) - sizeof(Packet);
    };
    char objId[0x40] = {};
};
