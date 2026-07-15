#pragma once

#include "Packet.h"

struct __attribute__((packed)) ChangeStagePacket : Packet {
    ChangeStagePacket() : Packet() {
        this->mType = PacketType::CHANGESTAGE;
        mPacketSize = sizeof(ChangeStagePacket) - sizeof(Packet);
    };
    char changeStage[0x30] = {};
    char changeID[0x10] = {};
    s8 scenarioNo = -1;
    u8 subScenarioType = -1;
    u16 extraDataFromServer = 0;
};