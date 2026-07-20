#pragma once

#include "sead/prim/seadSafeString.h"

#include "packets/Packet.h"

struct CaptureInf : public Packet {
    PacketType getType() override { return PacketType::CAPTUREINF; }

    std::vector<u8> serialize() override {
        PacketWriter writer(this);

        writer.writeString(hackName);

        return writer.finalize();
    }

    void deserialize(const std::vector<u8>& data) override {
        PacketReader reader(this, data.data(), data.size());

        reader.readString(hackName);

        reader.finalize();
    }

    sead::FixedSafeString<0x20> hackName;
};