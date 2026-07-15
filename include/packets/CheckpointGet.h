#pragma once

#include "packets/Packet.h"

struct __attribute__((packed)) CheckpointGet : Packet {
    CheckpointGet() : Packet() {
        this->mType = PacketType::CHECKPOINTGET;
        mPacketSize = sizeof(CheckpointGet) - sizeof(Packet);
    };
    char objId[0x40] = {};
};
