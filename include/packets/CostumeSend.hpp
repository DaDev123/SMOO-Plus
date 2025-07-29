#pragma once
#include "Packet.h"

#pragma pack(push, 1)
struct CoustumeSend : Packet {
    char BodyName[COSTUMEBUFSIZE];  // 0x20 bytes for body costume name
    char CapName[COSTUMEBUFSIZE];   // 0x20 bytes for cap costume name
    CoustumeSend() {
        mType = PacketType::COSTUMESEND;
        mPacketSize = sizeof(CoustumeSend) - sizeof(Packet);  // = 0x40 (64 bytes)
        
        // Initialize strings to empty
        memset(BodyName, 0, COSTUMEBUFSIZE);
        memset(CapName, 0, COSTUMEBUFSIZE);
    }
};
#pragma pack(pop)
    