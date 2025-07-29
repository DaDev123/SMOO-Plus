#pragma once
#include "Packet.h"

#pragma pack(push, 1)
struct ExtrasPacket : Packet {
    u8 InfiniteCapBounce = 0;
    u8 Noclip = 0;

    ExtrasPacket() {
        mType = PacketType::EXTRA;
        mPacketSize = sizeof(ExtrasPacket) - sizeof(Packet);  // = 2
    }
};
#pragma pack(pop)
