#pragma once

#include "packets/Packet.h"

struct __attribute__((packed)) PlayerConnect : Packet {
    PlayerConnect() : Packet() {
        this->mType = PacketType::PLAYERCON;
        mPacketSize = sizeof(PlayerConnect) - sizeof(Packet);
    };
    ConnectionTypes conType;
    u16 maxPlayerCount = USHRT_MAX;
    char clientName[COSTUMEBUFSIZE] = {};
};