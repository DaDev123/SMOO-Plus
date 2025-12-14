#pragma once

#include "Packet.h"

struct PACKED MessagePacket : Packet {
    MessagePacket() : Packet() {
        this->mType = PacketType::MESSAGE;
        mPacketSize = sizeof(MessagePacket) - sizeof(Packet);
    };

    short senderId = 0;
    short messageType = 0;
    char message[MESSAGESIZE] = {};
};