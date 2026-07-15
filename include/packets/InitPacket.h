#pragma once

#include "Packet.h"

struct __attribute__((packed)) InitPacket : Packet {
    InitPacket() : Packet() {
        this->mType = PacketType::CLIENTINIT;
        mPacketSize = sizeof(InitPacket) - sizeof(Packet);
    };
    u16 maxPlayers = 0;
};