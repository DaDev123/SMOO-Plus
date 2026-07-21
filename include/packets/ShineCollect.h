#pragma once

#include "packets/Packet.h"

struct ShineCollect : public Packet {
    PacketType getType() override { return PacketType::SHINECOLL; }

    PacketVector serialize() override {
        PacketWriter writer(this);

        writer.write(shineId);
        writer.write(isGrand);

        return writer.finalize();
    }

    void deserialize(const PacketVector& data) override {
        PacketReader reader(this, data.data(), data.size());

        reader.read(shineId);
        reader.read(isGrand);

        reader.finalize();
    }

    int shineId = -1;
    u8 isGrand = false;  // fake bool
};