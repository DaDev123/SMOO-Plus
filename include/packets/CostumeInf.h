#pragma once

#include "Packet.h"

// Structure for sending/receiving costume information (Mod <-> Server)
struct PACKED CostumeInf : Packet {
    CostumeInf() : Packet() {
        this->mType = PacketType::COSTUMEINF;
        mPacketSize = sizeof(CostumeInf) - sizeof(Packet);
    }
    CostumeInf(const char* body, const char* cap) : Packet() {
        this->mType = PacketType::COSTUMEINF;
        mPacketSize = sizeof(CostumeInf) - sizeof(Packet);
        if (body)
            strncpy(bodyModel, body, COSTUMEBUFSIZE - 1);
        bodyModel[COSTUMEBUFSIZE - 1] = '\0';
        if (cap)
            strncpy(capModel, cap, COSTUMEBUFSIZE - 1);
        capModel[COSTUMEBUFSIZE - 1] = '\0';
    }
    char bodyModel[COSTUMEBUFSIZE] = {};
    char capModel[COSTUMEBUFSIZE] = {};
};