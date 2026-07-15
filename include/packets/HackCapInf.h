#pragma once

#include "Packet.h"

struct __attribute__((packed)) HackCapInf : Packet {
    HackCapInf() : Packet() {
        this->mType = PacketType::HACKCAPINF;
        mPacketSize = sizeof(HackCapInf) - sizeof(Packet);
    };
    sead::Vector3f capPos;
    sead::Quatf capQuat;
    u8 isCapVisible = false;  // fake bool
    char capAnim[PACKBUFSIZE] = {};
    sead::Quatf capRotQuat;
};