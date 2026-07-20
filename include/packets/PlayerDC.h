#pragma once

#include "packets/Packet.h"

struct PlayerDC : public Packet {
    PacketType getType() override { return PacketType::PLAYERDC; }

    std::vector<u8> serialize() override {
        PacketWriter writer(this);

        return writer.finalize();
    }

    void deserialize(const std::vector<u8>& data) override {
        PacketReader reader(this, data.data(), data.size());

        reader.finalize();
    }
};