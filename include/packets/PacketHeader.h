#pragma once

#include "packets/Packet.h"

struct PacketHeader : public Packet {
    PacketType getType() override { return PacketType::UNKNOWN; }

    std::vector<u8> serialize() override { return std::vector<u8>(); }

    void deserialize(const std::vector<u8>& data) override {
        PacketReader reader(this, data.data(), data.size());

        reader.finalize();
    }
};