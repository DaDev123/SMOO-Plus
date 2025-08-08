#pragma once

#include "Packet.h"

struct PACKED ChangeCostume : Packet {
    ChangeCostume() : Packet() {
        this->mType = PacketType::CHANGECOSTUME;
        mPacketSize = sizeof(ChangeCostume) - sizeof(Packet);
    };
    ChangeCostume(const char* body, const char* cap) : Packet() {
        this->mType = PacketType::CHANGECOSTUME;
        mPacketSize = sizeof(ChangeCostume) - sizeof(Packet);
        strcpy(bodyModel, body);
        strcpy(capModel, cap);
    }
    char bodyModel[COSTUMEBUFSIZE] = {};
    char capModel[COSTUMEBUFSIZE] = {};
};