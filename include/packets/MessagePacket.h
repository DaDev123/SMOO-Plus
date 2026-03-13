#pragma once

#include "Packet.h"

struct PACKED MessagePacket : Packet {
    MessagePacket() : Packet() {
        this->mType = PacketType::MESSAGE;
        mPacketSize = sizeof(MessagePacket) - sizeof(Packet);
    };

    nn::account::Uid senderId = nn::account::Uid();  // User ID of the message sender
    int messageType = 0;                             // Changed from short to int (4 bytes)
    char message[MESSAGESIZE] = {};
};

enum MessageType : int { CHAT = 0, SYSTEM = 1, PRIVATE = 2 };