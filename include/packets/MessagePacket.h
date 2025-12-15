#pragma once

#include "Packet.h"

struct PACKED MessagePacket : Packet {
    MessagePacket() : Packet() {
        this->mType = PacketType::MESSAGE;
        mPacketSize = sizeof(MessagePacket) - sizeof(Packet);
    };

    uint senderId = 0;        // Changed from short to uint (4 bytes)
    int messageType = 0;      // Changed from short to int (4 bytes)
    char message[MESSAGESIZE] = {};
};